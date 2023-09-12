// Copyright RedFox Studio 2022

#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <volk.h>

#include <algorithm>
#include <vector>
#include <string>
#include <iterator>

#define VKFAILED(result) ((result != VK_SUCCESS ? true : false))
#define VKSUCCEDED(result) ((result == VK_SUCCESS ? true : false))

namespace VkUtils
{

inline const char*
VkErrorString(enum VkResult result)
{
    switch (result)
        {
            case VK_SUCCESS:
                return "VK_SUCCESS";
                break;
            case VK_NOT_READY:
                return "VK_NOT_READY";
                break;
            case VK_TIMEOUT:
                return "VK_TIMEOUT";
                break;
            case VK_EVENT_SET:
                return "VK_EVENT_SET";
                break;
            case VK_EVENT_RESET:
                return "VK_EVENT_RESET";
                break;
            case VK_INCOMPLETE:
                return "VK_INCOMPLETE";
                break;
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                return "VK_ERROR_OUT_OF_HOST_MEMORY";
                break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
                break;
            case VK_ERROR_INITIALIZATION_FAILED:
                return "VK_ERROR_INITIALIZATION_FAILED";
                break;
            case VK_ERROR_DEVICE_LOST:
                return "VK_ERROR_DEVICE_LOST";
                break;
            case VK_ERROR_MEMORY_MAP_FAILED:
                return "VK_ERROR_MEMORY_MAP_FAILED";
                break;
            case VK_ERROR_LAYER_NOT_PRESENT:
                return "VK_ERROR_LAYER_NOT_PRESENT";
                break;
            case VK_ERROR_EXTENSION_NOT_PRESENT:
                return "VK_ERROR_EXTENSION_NOT_PRESENT";
                break;
            case VK_ERROR_FEATURE_NOT_PRESENT:
                return "VK_ERROR_FEATURE_NOT_PRESENT";
                break;
            case VK_ERROR_INCOMPATIBLE_DRIVER:
                return "VK_ERROR_INCOMPATIBLE_DRIVER";
                break;
            case VK_ERROR_TOO_MANY_OBJECTS:
                return "VK_ERROR_TOO_MANY_OBJECTS";
                break;
            case VK_ERROR_FORMAT_NOT_SUPPORTED:
                return "VK_ERROR_FORMAT_NOT_SUPPORTED";
                break;
            case VK_ERROR_FRAGMENTED_POOL:
                return "VK_ERROR_FRAGMENTED_POOL";
                break;
            case VK_ERROR_UNKNOWN:
                return "VK_ERROR_UNKNOWN";
                break;
            default:
                return "UNKNOWN VK ERROR";
                break;
        }
};

inline std::vector<const char*>
convertLayerPropertiesToNames(const std::vector<VkLayerProperties>& layers)
{
    std::vector<const char*> names;

    std::transform(layers.cbegin(), layers.cend(), std::back_inserter(names), [](const VkLayerProperties& p) { return p.layerName; });

    return names;
};

inline std::vector<const char*>
convertExtensionPropertiesToNames(const std::vector<VkExtensionProperties>& layers)
{
    std::vector<const char*> names;

    std::transform(layers.cbegin(), layers.cend(), std::back_inserter(names), [](const VkExtensionProperties& p) { return p.extensionName; });

    return names;
};

inline std::vector<VkLayerProperties>
getInstanceLayerProperties()
{
    // Get supported layers by vulkan
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    return availableLayers;
};

inline std::vector<VkExtensionProperties>
getInstanceExtensionProperties()
{
    // Get supported layers by vulkan
    uint32_t extensionCount;
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, availableExtensions.data());

    return availableExtensions;
};

inline std::vector<VkExtensionProperties>
getDeviceExtensionProperties(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, "", &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, "", &extensionCount, availableExtensions.data());
    return availableExtensions;
};

inline std::vector<VkLayerProperties>
getDeviceLayerProperties(VkPhysicalDevice device)
{
    // Get supported layers by vulkan
    uint32_t layerCount = 0;
    vkEnumerateDeviceLayerProperties(device, &layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateDeviceLayerProperties(device, &layerCount, availableLayers.data());
    return availableLayers;
};

inline VkResult
enumeratePhysicalDevices(VkInstance vkInstance, std::vector<VkPhysicalDevice>& physicalDevices)
{
    physicalDevices.clear();

    uint32_t deviceCount = 0;
    // Get number of available physical devices
    {
        const VkResult result = vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);
        if (VKFAILED(result))
            {
                return result;
            }
    }

    if (deviceCount > 0)
        {
            // Enumerate the physical devices
            physicalDevices.resize(deviceCount);

            const VkResult result = vkEnumeratePhysicalDevices(vkInstance, &deviceCount, physicalDevices.data());
            if (VKFAILED(result))
                {
                    return result;
                }
        }

    return VkResult::VK_SUCCESS;
};

inline uint32_t
selectPhysycalDeviceOnHighestMemory(const std::vector<VkPhysicalDevice>& physicalDevices)
{
    // Select physical device to be used for the Vulkan example
    uint32_t selectedDevice = 0;

    // Multi-GPU - Select the device with highest memory available (usually the best gpu, but not always)
    if (physicalDevices.size() > 1)
        {
            uint64_t highestMemoryOfSelectedDevice = 0;
            for (size_t i = 0; i < physicalDevices.size(); i++)
                {
                    VkPhysicalDevice                 device       = physicalDevices[i];
                    VkPhysicalDeviceMemoryProperties deviceMemory = {};
                    vkGetPhysicalDeviceMemoryProperties(device, &deviceMemory);

                    VkPhysicalDeviceProperties prop{};
                    vkGetPhysicalDeviceProperties(device, &prop);
                    const std::string deviceName = prop.deviceName;

                    const std::string out = "Gpu index:" + std::to_string(i) + " with name:" + deviceName;

                    if (deviceMemory.memoryHeaps->size > highestMemoryOfSelectedDevice)
                        {
                            selectedDevice                = (uint32_t)i;
                            highestMemoryOfSelectedDevice = deviceMemory.memoryHeaps->size;
                        }
                }
        }
    return selectedDevice;
};

//@TODO can be unit tested
inline std::vector<const char*>
filterInclusive(const std::vector<const char*> source, const std::vector<const char*> included)
{
    std::vector<const char*> inclusive;
    std::copy_if(source.cbegin(), source.cend(), std::back_inserter(inclusive), [&included](const char* s1) {
        const auto found = std::find_if(included.begin(), included.end(), [s1](const char* s2) { return (strcmp(s1, s2) == 0); });
        return found != included.end();
    });

    return inclusive;
};

//@TODO can be unit tested
inline std::vector<const char*>
filterExclusive(const std::vector<const char*> source, const std::vector<const char*> excluded)
{
    std::vector<const char*> inclusive;
    std::copy_if(source.cbegin(), source.cend(), std::back_inserter(inclusive), [&excluded](const char* s1) {
        const auto found = std::find_if(excluded.begin(), excluded.end(), [s1](const char* s2) { return (strcmp(s1, s2) == 0); });
        return found == excluded.end();
    });

    return inclusive;
};

}