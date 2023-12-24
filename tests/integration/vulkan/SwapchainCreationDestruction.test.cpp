#include "WindowFixture.h"

#include "backend/vulkan/VulkanContextFactory.h"

TEST_F(WindowFixture, ShouldCreateAndDestroySwapchain)
{

    Fox::DContextConfig config;
    config.warningFunction = &WarningAssert;

    Fox::IContext* context = Fox::CreateVulkanContext(&config);
    ASSERT_NE(context, nullptr);

    Fox::WindowData windowData;
#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)
    windowData._hwnd     = glfwGetWin32Window(_window);
    windowData._instance = GetModuleHandle(NULL);
#elif defined(__linux__)
    windowData._display;
    windowData._window;
#endif

    uint32_t swapchain{};
    auto     presentMode = Fox::EPresentMode::IMMEDIATE_KHR;
    auto     format      = Fox::EFormat::B8G8R8A8_UNORM;
    swapchain            = context->CreateSwapchain(&windowData, presentMode, format);
    ASSERT_NE(swapchain, 0);

    context->DestroySwapchain(swapchain);

    delete context;
}