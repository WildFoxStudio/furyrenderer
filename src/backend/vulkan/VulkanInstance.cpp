// Copyright RedFox Studio 2022

#include "VulkanInstance.h"

#include "UtilsVK.h"
#include "asserts.h"

#define VOLK_IMPLEMENTATION // defined only once
#include "volk.h"

namespace Fox
{

VkResult
RIVulkanInstance::Init(const char* applicationName, std::vector<const char*> validationLayers, std::vector<const char*> extensions)
{
    furyassert(Instance == nullptr);

    VkApplicationInfo applicationInfo  = {};
    applicationInfo.pNext              = NULL;
    applicationInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.apiVersion         = VK_API_VERSION_1_2;
    applicationInfo.applicationVersion = 1;
    applicationInfo.pApplicationName   = applicationName;
    applicationInfo.pEngineName        = "RedFox Game Engine";

    VkInstanceCreateInfo createInfo    = {};
    createInfo.pNext                   = NULL;
    createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.flags                   = 0;
    createInfo.pApplicationInfo        = &applicationInfo;
    createInfo.enabledLayerCount       = (uint32_t)validationLayers.size();
    createInfo.ppEnabledLayerNames     = validationLayers.data();
    createInfo.enabledExtensionCount   = (uint32_t)extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();

    const VkResult result = vkCreateInstance(&createInfo, nullptr, &Instance);
    if (VKFAILED(result))
        {
            return result;
        }
    // Load initialize all function through vkGetInstanceProcAddr
    volkLoadInstance(Instance);

    return result;
}

void
RIVulkanInstance::Deinit()
{
#if _DEBUG
    if (_debugMessenger)
        {
            _destroyDebugUtilsMessenger();
        }
#endif
    furyassert(Instance != nullptr);
    vkDestroyInstance(Instance, nullptr);
}

VkResult
RIVulkanInstance::CreateSurfaceFromWindow(const WindowData& windowData, VkSurfaceKHR* surface)
{

#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)
    {
        VkWin32SurfaceCreateInfoKHR createInfo = {};
        createInfo.sType                       = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.pNext                       = NULL;
        createInfo.hinstance                   = windowData._instance;
        createInfo.hwnd                        = windowData._hwnd;
        const VkResult result                  = vkCreateWin32SurfaceKHR(Instance, &createInfo, NULL, surface);
        if (VKFAILED(result))
            {
                return result;
            }
    }
#elif defined(__linux__)
    {
        Fox::WindowPlatformLinuxSDL* win = static_cast<Fox::WindowPlatformLinuxSDL*>(window);
        furyassert(win);

        VkXlibSurfaceCreateInfoKHR createInfo = {};
        createInfo.sType                      = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.pNext                      = NULL;
        createInfo.dpy                        = windowData._display;
        createInfo.window                     = windowData._window;
        const VkResult result                 = vkCreateXlibSurfaceKHR(_vkInstance, &createInfo, NULL, surface);
        if (VKFAILED(result))
            {
                return result;
            }
    }
#else
#pragma Error "Platform is not supported!"
#endif

    return VK_SUCCESS;
}

void
RIVulkanInstance::DestroySurface(VkSurfaceKHR surface)
{
    vkDestroySurfaceKHR(Instance, surface, nullptr);
}

#ifdef _DEBUG
VkResult
RIVulkanInstance::CreateDebugUtilsMessenger(PFN_vkDebugUtilsMessengerCallbackEXT callback, void* userData)
{
    VkDebugUtilsMessengerCreateInfoEXT dbgInfo{};
    dbgInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    dbgInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    dbgInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    dbgInfo.pfnUserCallback = callback;
    dbgInfo.pUserData       = userData; // Optional

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT");
    furyassert(func);
    if (func != nullptr)
        {
            const VkResult result = func(Instance, &dbgInfo, nullptr, &_debugMessenger);
            return result;
        }

    return VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR;
}

void
RIVulkanInstance::_destroyDebugUtilsMessenger()
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT");
    furyassert(func);
    if (func != nullptr)
        {
            func(Instance, _debugMessenger, nullptr);
        }
}
#endif

}