// Copyright RedFox Studio 2022

#pragma once

#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
#else
#pragma error Platform not supported
#endif

#include "IContext.h"

#define VK_NO_PROTOTYPES

#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)
#define VK_USE_PLATFORM_WIN32_KHR
// #include <vulkan/vulkan_win32.h> // Include the Win32-specific extension header
#elif defined(__linux__)
#define VK_USE_PLATFORM_XLIB_KHR
// #include <vulkan/vulkan_xlib.h>
#endif
// Building There are multiple ways to use volk in your project:
//
//   You can just add volk.c to your build system.Note that the usual preprocessor defines that enable
//   Vulkan's platform-specific functions (VK_USE_PLATFORM_WIN32_KHR, VK_USE_PLATFORM_XLIB_KHR, VK_USE_PLATFORM_MACOS_MVK, etc) must be passed as desired to the compiler when building volk.c. You can
//   use volk in header -
// only fashion.Include volk.h wherever you want to use Vulkan functions.In exactly one source file
//   , define VOLK_IMPLEMENTATION before including volk.h.Do not build volk.c at all
//     in this case.This method of integrating volk makes it possible to set the platform defines mentioned above with
//     arbitrary(preprocessor) logic in your code.You can use provided CMake files
//   , with the usage detailed below.
#include <volk.h>

#include <vector>

namespace Fox
{
class RIVulkanInstance
{
  public:
    VkResult Init(const char* applicationName, std::vector<const char*> validationLayers, std::vector<const char*> extensions);
    void     Deinit();

    VkResult CreateSurfaceFromWindow(const WindowData& windowData, VkSurfaceKHR* surface);
    void     DestroySurface(VkSurfaceKHR surface);

#ifdef _DEBUG
    bool CreateDebugUtilsMessenger(PFN_vkDebugUtilsMessengerCallbackEXT callback);
#endif

    VkInstance Instance{}; /* Vulkan library handle */
#ifdef _DEBUG
    VkDebugUtilsMessengerEXT _debugMessenger{};

  private:
    void _destroyDebugUtilsMessenger();
#endif
};

}