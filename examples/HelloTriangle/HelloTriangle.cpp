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

        Fox::PipelineFormat pipelineFormat;
        Fox::DFramebufferAttachments attachments;
        attachments.ImageIds[0] = swapchainImages[0];

        uint32_t pipeline = context->CreatePipeline(shader, attachments, pipelineFormat);

        Fox::FramebufferId swapchainFbo = context->CreateSwapchainFramebuffer_DEPRECATED(swapchain);

        Fox::DRenderPassAttachment  colorAttachment(format,
        Fox::ESampleBit::COUNT_1_BIT,
        Fox::ERenderPassLoad::Clear,
        Fox::ERenderPassStore::Store,
        Fox::ERenderPassLayout::Undefined,
        Fox::ERenderPassLayout::Present,
        Fox::EAttachmentReference::COLOR_ATTACHMENT);
        Fox::DRenderPassAttachments renderPass;
        renderPass.Attachments.push_back(colorAttachment);

        while (!glfwWindowShouldClose(window))
            {
                // Keep running
                glfwPollEvents();

                int w, h;
                glfwGetWindowSize(window, &w, &h);
                Fox::DViewport viewport{ 0, 0, w, h, 0.f, 1.f };

                Fox::RenderPassData draw(swapchainFbo, viewport, renderPass);
                draw.ClearColor(1, 0, 0, 1);

                Fox::DrawCommand drawTriangle(shader);
                drawTriangle.Draw(triangle, 0, 3);

                draw.AddDrawCommand(std::move(drawTriangle));

                context->SubmitPass(std::move(draw));

                context->AdvanceFrame();
            }
        context->WaitDeviceIdle();
        context->DestroyShader(shader);
        context->DestroyBuffer(triangle);
        context->DestroySwapchain(swapchain);
        context->DestroyPipeline(pipeline);

        glfwDestroyWindow(window);

        glfwTerminate();
    }

    delete context;

    return 0;
}