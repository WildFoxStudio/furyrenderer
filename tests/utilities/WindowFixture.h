#pragma once

#include <gtest/gtest.h>

#define GLFW_INCLUDE_NONE // makes the GLFW header not include any OpenGL or
// OpenGL ES API header.This is useful in combination with an extension loading library.
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

static void
WarningAssert(const char* msg)
{
    ASSERT_NE(0, 0) << std::string(msg);
};

class WindowFixture : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        ASSERT_EQ(glfwInit(), GLFW_TRUE);

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // as Vulkan, there is no need to create a context
        _window = glfwCreateWindow(640, 480, "Integration test", NULL, NULL);
        ASSERT_NE(_window, nullptr);
    }

    void TearDown() override
    {

        glfwDestroyWindow(_window);

        glfwTerminate();
    }

    GLFWwindow* _window{};
};