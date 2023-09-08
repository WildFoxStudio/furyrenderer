// Copyright RedFox Studio 2022

#pragma once

#include "RICoreVk.h"
#include "UtilsVK.h"
#include <sstream>
#include <string.h>

#if defined(FOX_USE_VULKAN)

namespace Fox
{

inline VkResult
RICoreVk::EnumeratePhysicalDevices(VkInstance vkInstance, std::vector<VkPhysicalDevice>& physicalDevices) const
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
    check(deviceCount > 0);

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
}

inline uint32_t
RICoreVk::SelectPhysycalDeviceOnHighestMemory(const std::vector<VkPhysicalDevice>& physicalDevices) const
{
    check(physicalDevices.size() > 0);

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
                    std::cout << "Gpu" << i << ":" << deviceName << std::endl;

                    if (deviceMemory.memoryHeaps->size > highestMemoryOfSelectedDevice)
                        {
                            selectedDevice                = (uint32_t)i;
                            highestMemoryOfSelectedDevice = deviceMemory.memoryHeaps->size;
                        }
                }
        }
    return selectedDevice;
}

inline VkResult
RICoreVk::GetInstanceVersion(uint32_t& major, uint32_t& minor, uint32_t& patch) const
{
    auto FN_vkEnumerateInstanceVersion = PFN_vkEnumerateInstanceVersion(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));
    if (vkEnumerateInstanceVersion == nullptr)
        {
            return VkResult::VK_ERROR_EXTENSION_NOT_PRESENT;
        }

    uint32_t       instanceVersion = VK_API_VERSION_1_0;
    const VkResult result          = vkEnumerateInstanceVersion(&instanceVersion);
    if (VKFAILED(result))
        {
            return result;
        }

    // Extract version info
    major = VK_API_VERSION_MAJOR(instanceVersion);
    minor = VK_API_VERSION_MINOR(instanceVersion);
    patch = VK_API_VERSION_PATCH(instanceVersion);

    return VkResult::VK_SUCCESS;
}

inline std::string
RICoreVk::GetAdapterDescription() const
{
    const std::string deviceName     = VulkanDevice.DeviceProperties.deviceName;
    const uint32_t    deviceId       = VulkanDevice.DeviceProperties.deviceID;
    const uint32_t    driverVersion  = VulkanDevice.DeviceProperties.driverVersion;
    const uint32_t    vendorId       = VulkanDevice.DeviceProperties.vendorID;
    const uint32_t    maxAllocations = VulkanDevice.DeviceProperties.limits.maxMemoryAllocationCount;
    const uint32_t    maxSamplers    = VulkanDevice.DeviceProperties.limits.maxSamplerAllocationCount;

    std::stringstream ss;
    ss << "Device properties:\n";
    ss << "Device Name:" << deviceName << "\n";
    ss << "Device id:" << deviceId << "\n";
    ss << "Driver version:" << driverVersion << "\n";
    ss << "Vendor id:" << vendorId << "\n";
    ss << "Limits:\n";
    ss << "Max allocations count:" << maxAllocations << "\n";
    ss << "Max samplers count:" << maxSamplers << "\n";

    return ss.str();
}

inline size_t
RICoreVk::GetAdapterDedicatedVideoMemory() const
{
    return VulkanDevice.DeviceMemory.memoryHeaps->size;
}

inline VkResult
RICoreVk::GetPhysicalDeviceSupportSurfaceKHR(VkPhysicalDevice device, uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32* isSupported) const
{
    check(device != nullptr);
    check(surface != nullptr);
    *isSupported = VK_FALSE;

    const VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(device, queueFamilyIndex, surface, isSupported);

    return result;
}

inline std::vector<const char*>
RICoreVk::CheckInstanceSupportedValidationLayers(std::vector<const char*> validationLayers, std::vector<const char*>& unsupportedValidationLayers) const
{
    // Get only supported validation layers availability
    std::vector<const char*> supportedValidationLayers;
    {
        // Get supported layers by vulkan
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        check(layerCount > 0);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        // Filter only layers that are actually supported
        supportedValidationLayers = FilterAvailableByPredicate(validationLayers, availableLayers, [](const char* s1, const VkLayerProperties& s2) { return (strcmp(s1, s2.layerName) == 0); });
        if (supportedValidationLayers.size() != validationLayers.size())
            {
                unsupportedValidationLayers = FilterDifference(validationLayers, supportedValidationLayers);
            }
    }

    return supportedValidationLayers;
}

inline std::vector<const char*>
RICoreVk::CheckInstanceSupportedExtensions(std::vector<const char*> extensions, std::vector<const char*>& unsupportedExtensions) const
{
    // Get only supported extensions availability
    std::vector<const char*> supportedExtensions;
    {
        // Get supported layers by vulkan
        uint32_t extensionCount;
        vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, nullptr);
        check(extensionCount > 0);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, availableExtensions.data());

        // Filter only layers that are actually supported
        supportedExtensions = FilterAvailableByPredicate(extensions, availableExtensions, [](const char* s1, const VkExtensionProperties& s2) { return (strcmp(s1, s2.extensionName) == 0); });
        if (supportedExtensions.size() != extensions.size())
            {
                unsupportedExtensions = FilterDifference(extensions, supportedExtensions);
            }
    }
    return supportedExtensions;
}

inline std::vector<const char*>
RICoreVk::CheckPhysicalDeviceSupportedValidationLayers(VkPhysicalDevice physicalDevice, std::vector<const char*> validationLayers, std::vector<const char*>& unsupportedValidationLayers) const
{
    // Get only supported validation layers availability
    std::vector<const char*> supportedValidationLayers;
    {
        // Get supported layers by vulkan
        uint32_t layerCount = 0;
        vkEnumerateDeviceLayerProperties(physicalDevice, &layerCount, nullptr);
        check(layerCount > 0);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateDeviceLayerProperties(physicalDevice, &layerCount, availableLayers.data());

        // Filter only layers that are actually supported
        supportedValidationLayers = FilterAvailableByPredicate(validationLayers, availableLayers, [](const char* s1, const VkLayerProperties& s2) { return (strcmp(s1, s2.layerName) == 0); });
        if (supportedValidationLayers.size() != validationLayers.size())
            {
                unsupportedValidationLayers = FilterDifference(validationLayers, supportedValidationLayers);
            }
    }

    return supportedValidationLayers;
}

inline std::vector<const char*>
RICoreVk::CheckPhysicalDeviceSupportedExtensions(VkPhysicalDevice physicalDevice, std::vector<const char*> extensions, std::vector<const char*>& unsupportedExtensions) const
{
    // Get only supported extensions availability
    std::vector<const char*> supportedExtensions;
    {
        // Get supported layers by vulkan
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(physicalDevice, "", &extensionCount, nullptr);
        check(extensionCount > 0);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, "", &extensionCount, availableExtensions.data());

        // Filter only layers that are actually supported
        supportedExtensions = FilterAvailableByPredicate(extensions, availableExtensions, [](const char* s1, const VkExtensionProperties& s2) { return (strcmp(s1, s2.extensionName) == 0); });
        if (supportedExtensions.size() != extensions.size())
            {
                unsupportedExtensions = FilterDifference(extensions, supportedExtensions);
            }
    }
    return supportedExtensions;
}

}

#endif
