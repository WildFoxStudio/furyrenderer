// Copyright RedFox Studio 2022

#include "VulkanInstance.h"

#include "Core/utils/debug.h"
#include "UtilsVK.h"

namespace Fox
{

bool
RIVulkanInstance::Init(const char* applicationName, std::vector<const char*> validationLayers, std::vector<const char*> extensions)
{
    check(Instance == nullptr);

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
            return false;
        }
    // Load initialize all function through vkGetInstanceProcAddr
    volkLoadInstance(Instance);

    return true;
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
    check(Instance != nullptr);
    vkDestroyInstance(Instance, nullptr);
}

VkSurfaceKHR
RIVulkanInstance::CreateSurfaceFromWindow(AWindow* window)
{
    VkSurfaceKHR surface{};

#if defined(FOX_WINDOWS)
    {
        Fox::WindowPlatformWin* win = static_cast<Fox::WindowPlatformWin*>(window);
        check(win);

        VkWin32SurfaceCreateInfoKHR createInfo = {};
        createInfo.sType                       = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.pNext                       = NULL;
        createInfo.hinstance                   = win->GetInstance();
        createInfo.hwnd                        = win->GetHwnd();
        const VkResult result                  = vkCreateWin32SurfaceKHR(Instance, &createInfo, NULL, &surface);
        if (VKFAILED(result))
            {
                throw std::runtime_error(VkErrorString(result));
            }
    }
#elif defined(FOX_LINUX)
    {
        Fox::WindowPlatformLinuxSDL* win = static_cast<Fox::WindowPlatformLinuxSDL*>(window);
        check(win);

        VkXlibSurfaceCreateInfoKHR createInfo = {};
        createInfo.sType                      = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.pNext                      = NULL;
        createInfo.dpy                        = win->GetDisplay();
        createInfo.window                     = win->GetWindow();
        const VkResult result                 = vkCreateXlibSurfaceKHR(_vkInstance, &createInfo, NULL, &surface);
        if (VKFAILED(result))
            {
                throw std::runtime_error(VkErrorString(result));
            }
    }
#else
#pragma Error "Platform is not supported!"
#endif

    return surface;
}

void
RIVulkanInstance::DestroySurface(VkSurfaceKHR surface)
{
    vkDestroySurfaceKHR(Instance, surface, nullptr);
}

#ifdef _DEBUG
bool
RIVulkanInstance::CreateDebugUtilsMessenger(PFN_vkDebugUtilsMessengerCallbackEXT callback)
{
    VkDebugUtilsMessengerCreateInfoEXT dbgInfo{};
    dbgInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    dbgInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    dbgInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    dbgInfo.pfnUserCallback = callback;
    dbgInfo.pUserData       = nullptr; // Optional

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT");
    check(func);
    if (func != nullptr)
        {
            return func(Instance, &dbgInfo, nullptr, &_debugMessenger);
            return true;
        }

    return false;
}

void
RIVulkanInstance::_destroyDebugUtilsMessenger()
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT");
    check(func);
    if (func != nullptr)
        {
            func(Instance, _debugMessenger, nullptr);
        }
}
#endif

}