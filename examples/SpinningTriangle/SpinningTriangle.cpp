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

        Fox::DSwapchain swapchain;
        auto            presentMode = Fox::EPresentMode::IMMEDIATE_KHR;
        auto            format      = Fox::EFormat::B8G8R8A8_UNORM;
        if (!context->CreateSwapchain(&windowData, presentMode, format, &swapchain))
            {
                throw std::runtime_error("Failed to CreateSwapchain");
            }

        // Create vertex layout
        Fox::VertexLayoutInfo   position("SV_POSITION", Fox::EFormat::R32G32B32_FLOAT, 0, Fox::EVertexInputClassification::PER_VERTEX_DATA);
        Fox::VertexLayoutInfo   color("Color0", Fox::EFormat::R32G32B32A32_FLOAT, 3 * sizeof(float), Fox::EVertexInputClassification::PER_VERTEX_DATA);
        Fox::DVertexInputLayout vertexLayout = context->CreateVertexLayout({ position, color });
        constexpr uint32_t      stride       = 7 * sizeof(float);

        Fox::ShaderSource shaderSource;
        shaderSource.VertexLayout            = vertexLayout;
        shaderSource.VertexStride            = stride;
        shaderSource.SourceCode.VertexShader = ReadBlobUnsafe("vertex.spv");
        shaderSource.SourceCode.PixelShader  = ReadBlobUnsafe("fragment.spv");
        shaderSource.ColorAttachments.push_back(format);

        shaderSource.SetsLayout.SetsLayout[0].insert({ 0, Fox::ShaderDescriptorBindings("MatrixUbo", Fox::EBindingType::UNIFORM_BUFFER_OBJECT, sizeof(float) * 16, 1, Fox::EShaderStage::VERTEX) });

        Fox::DShader shader = context->CreateShader(shaderSource);

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
        Fox::DBuffer     triangle = context->CreateVertexBuffer(bufSize);

        Fox::CopyDataCommand copy;
        copy.CopyVertex(triangle, 0, (void*)ndcTriangle.data(), bufSize);
        context->SubmitCopy(std::move(copy));

        Fox::DFramebuffer swapchainFbo = context->CreateSwapchainFramebuffer_DEPRECATED(swapchain);

        Fox::DRenderPassAttachment  colorAttachment(format,
        Fox::ESampleBit::COUNT_1_BIT,
        Fox::ERenderPassLoad::Clear,
        Fox::ERenderPassStore::Store,
        Fox::ERenderPassLayout::Undefined,
        Fox::ERenderPassLayout::Present,
        Fox::EAttachmentReference::COLOR_ATTACHMENT);
        Fox::DRenderPassAttachments renderPass;
        renderPass.Attachments.push_back(colorAttachment);

        Fox::DBuffer transformUniformBuffer = context->CreateUniformBuffer(sizeof(float) * 16);
        float        yaw{ 0 };

        while (!glfwWindowShouldClose(window))
            {
                // Keep running
                glfwPollEvents();

                int w, h;
                glfwGetWindowSize(window, &w, &h);
                Fox::DViewport viewport{ 0, 0, w, h, 0.f, 1.f };

                {
                    yaw += 0.01f;
                    const float sinA = std::sin(yaw);
                    const float cosA = std::cos(yaw);

                    float                matrix[4][4]{ { cosA, 0, -sinA, 0 }, { 0, 1, 0, 0 }, { sinA, 0, cosA, 0 }, { 0, 0, 0, 1 } };
                    Fox::CopyDataCommand copy;
                    copy.CopyUniformBuffer(transformUniformBuffer, &matrix[0][0], sizeof(float) * 16);
                    context->SubmitCopy(std::move(copy));
                }

                Fox::RenderPassData draw(swapchainFbo, viewport, renderPass);
                draw.ClearColor(1, 0, 0, 1);

                Fox::DrawCommand drawTriangle(shader);
                drawTriangle.BindBufferUniformBuffer(0, 0, transformUniformBuffer);
                drawTriangle.Draw(triangle, 0, 3);

                draw.AddDrawCommand(std::move(drawTriangle));

                context->SubmitPass(std::move(draw));

                context->AdvanceFrame();
            }
        context->DestroyShader(shader);
        context->DestroyVertexBuffer(triangle);
        context->DestroyUniformBuffer(transformUniformBuffer);

        context->DestroySwapchain(swapchain);

        glfwDestroyWindow(window);

        glfwTerminate();
    }

    delete context;

    return 0;
}