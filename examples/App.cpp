#include "App.h"

// Include the vulkan context factory
#include "backend/vulkan/VulkanContextFactory.h"

#include <iostream>
#include <stdexcept>

void
LogMessages(const char* msg)
{
    std::cout << msg << std::endl;
};

App::App()
{

    // Init window
    {
        if (!glfwInit())
            {
                throw std::runtime_error("Failed to glfwInit");
            }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // as Vulkan, there is no need to create a context
        _window = glfwCreateWindow(640, 480, "App", NULL, NULL);
        if (!_window)
            {
                throw std::runtime_error("Failed to glfwCreateWindow");
            }
    }
    // Initialize renderer
    {
        Fox::DContextConfig config;
        config.logOutputFunction = &LogMessages;
        config.warningFunction   = &LogMessages;

        _ctx = Fox::CreateVulkanContext(&config);
    }
    // Setup swapchain
    {
        _windowData._hwnd     = glfwGetWin32Window(_window);
        _windowData._instance = GetModuleHandle(NULL);

        auto presentMode = Fox::EPresentMode::IMMEDIATE_KHR;
        auto format      = Fox::EFormat::B8G8R8A8_UNORM;
        _swapchain       = _ctx->CreateSwapchain(&_windowData, presentMode, format);
        if (_swapchain == NULL)
            {
                throw std::runtime_error("Failed to CreateSwapchain");
            }
    }
    // Create per frame data
    for (uint32_t i = 0; i < MAX_FRAMES; i++)
        {
            _frameData[i].Fence                   = _ctx->CreateFence(true); // already signaled
            _frameData[i].CmdPool                 = _ctx->CreateCommandPool();
            _frameData[i].Cmd                     = _ctx->CreateCommandBuffer(_frameData[i].CmdPool);
            _frameData[i].ImageAvailableSemaphore = _ctx->CreateGpuSemaphore();
            _frameData[i].WorkFinishedSemaphore   = _ctx->CreateGpuSemaphore();
            _frameData[i].SwapchainRenderTarget   = _ctx->GetSwapchainRenderTargets(_swapchain)[i];
        }
}

App ::~App()
{
    _ctx->WaitDeviceIdle();
    for (uint32_t i = 0; i < MAX_FRAMES; i++)
        {
            _ctx->DestroyFence(_frameData[i].Fence);
            _ctx->DestroyCommandBuffer(_frameData[i].Cmd);
            _ctx->DestroyCommandPool(_frameData[i].CmdPool);
            _ctx->DestroyGpuSemaphore(_frameData[i].ImageAvailableSemaphore);
            _ctx->DestroyGpuSemaphore(_frameData[i].WorkFinishedSemaphore);
        }

    glfwDestroyWindow(_window);
    glfwTerminate();
}

void
App::Run()
{
    while (!glfwWindowShouldClose(_window))
        {
            // Keep running
            glfwPollEvents();

            int w, h;
            glfwGetWindowSize(_window, &w, &h);

            PerFrameData& data = _frameData[_frameIndex];

            // Wait for fence
            if (!_ctx->IsFenceSignaled(data.Fence))
                {
                    _ctx->WaitForFence(data.Fence, 0xffffff);
                }

            _ctx->ResetCommandPool(data.CmdPool);

            const bool success = _ctx->SwapchainAcquireNextImageIndex(_swapchain, 0xFFFFFFF, data.ImageAvailableSemaphore, &_swapchainImageIndex);
            if (!success)
                {
                    _ctx->WaitDeviceIdle();
                    _ctx->DestroySwapchain(_swapchain);
                    _swapchain = _ctx->CreateSwapchain(&_windowData, presentMode, format);
                    if (_swapchain == NULL)
                        {
                            throw std::runtime_error("Failed to CreateSwapchain");
                        }
                    for (uint32_t i = 0; i < MAX_FRAMES; i++)
                        {
                            _frameData[i].SwapchainRenderTarget = _ctx->GetSwapchainRenderTargets(_swapchain)[i];
                        }
                    _ctx->SwapchainAcquireNextImageIndex(_swapchain, 0xFFFFFFF, data.ImageAvailableSemaphore, &_swapchainImageIndex);
                }

            // Perform drawing
            Draw(data.Cmd, w, h);

            _ctx->ResetFence(data.Fence);
            _ctx->QueueSubmit({ data.ImageAvailableSemaphore }, { data.WorkFinishedSemaphore }, { data.Cmd }, data.Fence);

            _ctx->QueuePresent(_swapchain, _swapchainImageIndex, { data.WorkFinishedSemaphore });
        }
}