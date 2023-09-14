// Copyright RedFox Studio 2022

#include <iostream>

#include "IContext.h"
#include "backend/vulkan/VulkanContextFactory.h"

#include <iostream>

#define GLFW_INCLUDE_NONE // makes the GLFW header not include any OpenGL or
// OpenGL ES API header.This is useful in combination with an extension loading library.
#include <GLFW/glfw3.h>

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
        GLFWwindow* window = glfwCreateWindow(640, 480, "My Title", NULL, NULL);
        if (!window)
            {
                throw std::runtime_error("Failed to glfwCreateWindow");
            }

        while (!glfwWindowShouldClose(window))
            {
                // Keep running
                glfwPollEvents();
            }

        glfwDestroyWindow(window);

        // context->CreateSwapchain()
        glfwTerminate();
    }

    delete context;

    return 0;
}