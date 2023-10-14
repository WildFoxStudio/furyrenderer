// Copyright RedFox Studio 2022

#include <iostream>

#include "IContext.h"
#include "backend/vulkan/VulkanContextFactory.h"

#include <iostream>

#define GLFW_INCLUDE_NONE // makes the GLFW header not include any OpenGL or
// OpenGL ES API header.This is useful in combination with an extension loading library.
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <array>
#include <fstream>
#include <string>

void
Log(const char* msg)
{
    std::cout << msg << std::endl;
};

inline std::vector<unsigned char>
ReadBlobUnsafe(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file.is_open())
        return {};

    std::ifstream::pos_type pos = file.tellg();

    // What happens if the OS supports really big files.
    // It may be larger than 32 bits?
    // This will silently truncate the value/
    std::streamoff length = pos;

    std::vector<unsigned char> bytes(length);
    if (pos > 0)
        { // Manuall memory management.
            // Not a good idea use a container/.
            file.seekg(0, std::ios::beg);
            file.read((char*)bytes.data(), length);
        }
    file.close();
    return bytes;
};

int
main()
{

    Fox::DContextConfig config;
    config.logOutputFunction = &Log;
    config.warningFunction   = &Log;

    Fox::IContext* context = Fox::CreateVulkanContext(&config);

    {
        if (!glfwInit())
            {
                throw std::runtime_error("Failed to glfwInit");
            }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // as Vulkan, there is no need to create a context
        GLFWwindow* window = glfwCreateWindow(640, 480, "My Title", NULL, NULL);
        if (!window)
            {
                throw std::runtime_error("Failed to glfwCreateWindow");
            }
        Fox::WindowData windowData;
        windowData._hwnd     = glfwGetWin32Window(window);
        windowData._instance = GetModuleHandle(NULL);

        Fox::SwapchainId swapchain;
        auto             presentMode = Fox::EPresentMode::IMMEDIATE_KHR;
        auto             format      = Fox::EFormat::B8G8R8A8_UNORM;
        swapchain                    = context->CreateSwapchain(&windowData, presentMode, format);
        if (swapchain == NULL)
            {
                throw std::runtime_error("Failed to CreateSwapchain");
            }

        std::vector<Fox::ImageId> swapchainImages = context->GetSwapchainImages(swapchain);

        // Create vertex layout
        Fox::VertexLayoutInfo    position("SV_POSITION", Fox::EFormat::R32G32B32_FLOAT, 0, Fox::EVertexInputClassification::PER_VERTEX_DATA);
        Fox::VertexLayoutInfo    color("Color0", Fox::EFormat::R32G32B32A32_FLOAT, 3 * sizeof(float), Fox::EVertexInputClassification::PER_VERTEX_DATA);
        Fox::VertexInputLayoutId vertexLayout = context->CreateVertexLayout({ position, color });
        constexpr uint32_t       stride       = 7 * sizeof(float);

        Fox::ShaderSource shaderSource;
        shaderSource.VertexLayout            = vertexLayout;
        shaderSource.VertexStride            = stride;
        shaderSource.SourceCode.VertexShader = ReadBlobUnsafe("vertex.spv");
        shaderSource.SourceCode.PixelShader  = ReadBlobUnsafe("fragment.spv");
        shaderSource.ColorAttachments        = 1;

        Fox::ShaderId shader = context->CreateShader(shaderSource);

        // clang-format off
        constexpr std::array<float, 21> ndcTriangle{            
            -1, -1, 0.5,//pos
            0, 1, 0, 1,// color
            1, -1, 0.5,//pos
            0, 0, 1, 1,// color
            0, 1, 0.5,//pos
            0, 1, 1, 1// color
        };
        // clang-format on
        constexpr size_t bufSize  = sizeof(float) * ndcTriangle.size();
        Fox::BufferId    triangle = context->CreateBuffer(bufSize, Fox::VERTEX_INDEX_BUFFER, Fox::EMemoryUsage::RESOURCE_MEMORY_USAGE_CPU_ONLY);

        {
            // Copy vertices
            void* triangleData = context->BeginMapBuffer(triangle);
            memcpy(triangleData, (void*)ndcTriangle.data(), bufSize);
            context->EndMapBuffer(triangle);
        }

        std::vector<uint32_t> swapchainRenderTargets = context->GetSwapchainRenderTargets(swapchain);

        Fox::PipelineFormat          pipelineFormat;
        Fox::DFramebufferAttachments attachments;
        attachments.RenderTargets[0] = swapchainRenderTargets[0];

        uint32_t pipeline = context->CreatePipeline(shader, attachments, pipelineFormat);

        std::vector<uint32_t> commandPools;
        commandPools.push_back(context->CreateCommandPool());
        commandPools.push_back(context->CreateCommandPool());

        std::vector<uint32_t> cmds;
        cmds.push_back(context->CreateCommandBuffer(commandPools[0]));
        cmds.push_back(context->CreateCommandBuffer(commandPools[1]));

        std::vector<uint32_t> fences;
        fences.push_back(context->CreateFence(true));
        fences.push_back(context->CreateFence(true));

        std::vector<uint32_t> imageAvailableSemaphore;
        imageAvailableSemaphore.push_back(context->CreateGpuSemaphore());
        imageAvailableSemaphore.push_back(context->CreateGpuSemaphore());

        std::vector<uint32_t> workFinishedSemaphore;
        workFinishedSemaphore.push_back(context->CreateGpuSemaphore());
        workFinishedSemaphore.push_back(context->CreateGpuSemaphore());

        Fox::DRenderPassAttachment  colorAttachment(format,
        Fox::ESampleBit::COUNT_1_BIT,
        Fox::ERenderPassLoad::Clear,
        Fox::ERenderPassStore::Store,
        Fox::ERenderPassLayout::Undefined,
        Fox::ERenderPassLayout::Present,
        Fox::EAttachmentReference::COLOR_ATTACHMENT);
        Fox::DRenderPassAttachments renderPass;
        renderPass.Attachments.push_back(colorAttachment);

        uint32_t frameIndex     = 0;
        uint32_t swapchainIndex = 0;

        while (!glfwWindowShouldClose(window))
            {
                // Keep running
                glfwPollEvents();

                int w, h;
                glfwGetWindowSize(window, &w, &h);
                Fox::DViewport viewport{ 0, 0, w, h, 0.f, 1.f };

                if (!context->IsFenceSignaled(fences[frameIndex]))
                    {
                        context->WaitForFence(fences[frameIndex], 0xffffff);
                    }

                context->ResetCommandPool(commandPools[frameIndex]);

                const bool success = context->SwapchainAcquireNextImageIndex(swapchain, 0xFFFFFFF, imageAvailableSemaphore[frameIndex], &swapchainIndex);

                auto cmd = cmds[frameIndex];
                context->BeginCommandBuffer(cmd);

                Fox::DFramebufferAttachments attachments;
                attachments.RenderTargets[0] = swapchainRenderTargets[swapchainIndex];

                Fox::DLoadOpPass loadOp;
                loadOp.LoadColor[0]         = Fox::ERenderPassLoad::Clear;
                loadOp.ClearColor->color    = { 1, 1, 1, 1 };
                loadOp.StoreActionsColor[0] = Fox::ERenderPassStore::Store;
                context->BindRenderTargets(cmd, attachments, loadOp);

                Fox::RenderTargetBarrier presentBarrier;
                presentBarrier.RenderTarget  = swapchainRenderTargets[swapchainIndex];
                presentBarrier.mArrayLayer   = 1;
                presentBarrier.mCurrentState = Fox::EResourceState::RENDER_TARGET;
                presentBarrier.mNewState     = Fox::EResourceState::PRESENT;

                context->ResourceBarrier(cmd, 0, nullptr, 0, nullptr, 1, &presentBarrier);

                context->EndCommandBuffer(cmd);


                context->ResetFence(fences[frameIndex]);
                context->QueueSubmit({ imageAvailableSemaphore[frameIndex] }, { workFinishedSemaphore[frameIndex] }, { cmd }, fences[frameIndex]);

               

                context->QueuePresent(swapchain, swapchainIndex, { workFinishedSemaphore[frameIndex] });
            }
        context->WaitDeviceIdle();
        context->DestroyShader(shader);
        context->DestroyBuffer(triangle);
        context->DestroySwapchain(swapchain);
        context->DestroyPipeline(pipeline);

        for (size_t i = 0; i < commandPools.size(); i++)
            {
                context->DestroyCommandPool(commandPools[i]);
                context->DestroyCommandBuffer(cmds[i]);
            }

        for (auto fence : fences)
            context->DestroyFence(fence);

        for (auto semaphore : imageAvailableSemaphore)
            context->DestroyGpuSemaphore(semaphore);

        for (auto semaphore : workFinishedSemaphore)
            context->DestroyGpuSemaphore(semaphore);

        glfwDestroyWindow(window);

        glfwTerminate();
    }

    delete context;

    return 0;
}