// Copyright RedFox Studio 2022

#pragma once

#if defined(FOX_USE_VULKAN)
#include "RIContextVk.h"

#include "RenderInterface/vulkan/UtilsVK.h"
#include <iostream>
#include <vector>

namespace Fox
{

inline static VKAPI_ATTR VkBool32 VKAPI_CALL
DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            std::cerr << "Validation layer[WARNING]: " << pCallbackData->pMessage << std::endl;
        }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            std::cerr << "Validation layer[ERROR]: " << pCallbackData->pMessage << std::endl;
        }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        {
            std::cerr << "Validation layer[VERBOSE]: " << pCallbackData->pMessage << std::endl;
        }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        {
            std::cerr << "Validation layer[INFO]: " << pCallbackData->pMessage << std::endl;
        }

    // check(false);
    return VK_FALSE;
}

inline std::vector<const char*>
RIContextVk::_getInstanceSupportedExtensions(const std::vector<const char*>& extentions)
{
    std::vector<const char*> unsupportedExtensions;
    std::vector<const char*> supportedExtensions = CheckInstanceSupportedExtensions(extentions, unsupportedExtensions);
    // check(supportedExtensions.size() == _instanceExtensionNames.size());

    {
        std::cout << "Available extensions requested:" << supportedExtensions.size() << "/" << extentions.size() << std::endl;
        // Print not supported validation layers
        if (supportedExtensions.size() != extentions.size())
            {
                std::cout << "Unsupported instance extensions:\n";
                for (const char* extensionName : unsupportedExtensions)
                    {
                        std::cout << extensionName << "\n";
                    }
                std::cout << std::endl;
            }
    }
    return supportedExtensions;
}

inline std::vector<const char*>
RIContextVk::_getInstanceSupportedValidationLayers(const std::vector<const char*>& validationLayers)
{
    std::vector<const char*> unsupportedValidationLayers;
    std::vector<const char*> supportedValidationLayers = CheckInstanceSupportedValidationLayers(validationLayers, unsupportedValidationLayers);
    // check(supportedValidationLayers.size() == _validationLayers.size());

    {
        std::cout << "Available validation layers requested:" << supportedValidationLayers.size() << "/" << validationLayers.size() << std::endl;
        // Print not supported validation layers
        if (supportedValidationLayers.size() != validationLayers.size())
            {
                std::cout << "Unsupported instance validation layers:\n";
                for (const char* layerName : unsupportedValidationLayers)
                    {
                        std::cout << layerName << "\n";
                    }
                std::cout << std::endl;
            }
    }

    return supportedValidationLayers;
}


inline void
RIContextVk::_printVersion()
{
    uint32_t       max, min, patch;
    const VkResult result = GetInstanceVersion(max, min, patch);
    if (VKFAILED(result))
        {
            std::cout << "Failed to request instance version for:" << VkErrorString(result) << "\n";
        }
    else
        {
            std::cout << "Created instance with Vulkan version:" << max << "." << min << "." << patch << "\n";
        }
}

inline VkPhysicalDevice
RIContextVk::_queryBestPhysicalDevice()
{
    std::vector<VkPhysicalDevice> physicalDevices;
    const VkResult                result = EnumeratePhysicalDevices(VulkanInstance.Instance, physicalDevices);
    if (VKFAILED(result))
        {
            std::cout << "Failed to enumerate physical devices:" << VkErrorString(result) << "\n";
            throw std::runtime_error(VkErrorString(result));
        }
    // Select gpu
    const uint32_t deviceIndex = SelectPhysycalDeviceOnHighestMemory(physicalDevices);
    return physicalDevices.at(deviceIndex);
}

inline std::vector<const char*>
RIContextVk::_getDeviceSupportedExtensions(VkPhysicalDevice physicalDevice, const std::vector<const char*>& extentions)
{

    std::vector<const char*> deviceUnsupportedExtensions;
    std::vector<const char*> deviceSupportedExtensions = RICoreVk::CheckPhysicalDeviceSupportedExtensions(physicalDevice, extentions, deviceUnsupportedExtensions);

    std::cout << "Available device extensions requested:" << deviceSupportedExtensions.size() << "/" << extentions.size() << std::endl;
    // Print not supported validation layers
    if (deviceSupportedExtensions.size() != extentions.size())
        {
            std::cout << "Unsupported device extensions:\n";
            for (const char* extensionName : deviceUnsupportedExtensions)
                {
                    std::cout << extensionName << "\n";
                }
            std::cout << std::endl;
        }

    return deviceSupportedExtensions;
}

inline std::vector<const char*>
RIContextVk::_getDeviceSupportedValidationLayers(VkPhysicalDevice physicalDevice, const std::vector<const char*>& validationLayers)
{
    std::vector<const char*> deviceUnsupportedValidationLayers;
    std::vector<const char*> deviceSupportedValidationLayers = RICoreVk::CheckPhysicalDeviceSupportedValidationLayers(physicalDevice, validationLayers, deviceUnsupportedValidationLayers);

    std::cout << "Available device validation layers requested:" << deviceSupportedValidationLayers.size() << "/" << validationLayers.size() << std::endl;
    // Print not supported validation layers
    if (deviceSupportedValidationLayers.size() != validationLayers.size())
        {
            std::cout << "Unsupported device validation layers:\n";
            for (const char* layerName : deviceUnsupportedValidationLayers)
                {
                    std::cout << layerName << "\n";
                }
            std::cout << std::endl;
        }

    return deviceSupportedValidationLayers;
}

inline void
RIContextVk::WaitGpuIdle() const
{
    vkDeviceWaitIdle(VulkanDevice.Device);
}
}

#endif