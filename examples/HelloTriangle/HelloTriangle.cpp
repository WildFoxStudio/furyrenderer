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

void
Log(const char* msg)
{
    std::cout << msg << std::endl;
};

int
main()
{

    Fox::DContextConfig config;
    config.logOutputFunction = &Log;

    Fox::IContext* context = Fox::CreateVulkanContext(&config);

    {
        if (!glfwInit())
            {
                throw std::runtime_error("Failed to glfwInit");
            }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);// as Vulkan, there is no need to create a context
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

        while (!glfwWindowShouldClose(window))
            {
                // Keep running
                glfwPollEvents();
            }

        context->DestroySwapchain(swapchain);

        glfwDestroyWindow(window);

        glfwTerminate();
    }

    delete context;

    return 0;
}