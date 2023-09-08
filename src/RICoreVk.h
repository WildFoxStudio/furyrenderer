// Copyright RedFox Studio 2022

#pragma once

#if defined(FOX_USE_VULKAN)

// std
#include <algorithm>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "Core/utils/Filter.h"
#include "Core/window/WindowBase.h"
#include "RIComponents.h"

#include "VulkanDevice12.h"

namespace Fox
{
typedef RIVulkanDevice12 CVulkanDevice;

class RICoreVk : public ARIFactory
{
  public:
    RICoreVk();
    virtual ~RICoreVk();

    inline VkResult                 EnumeratePhysicalDevices(VkInstance vkInstance, std::vector<VkPhysicalDevice>& physicalDevices) const;
    inline uint32_t                 SelectPhysycalDeviceOnHighestMemory(const std::vector<VkPhysicalDevice>& physicalDevices) const;
    inline std::vector<const char*> CheckInstanceSupportedValidationLayers(std::vector<const char*> validationLayers, std::vector<const char*>& unsupportedValidationLayers) const;
    inline std::vector<const char*> CheckInstanceSupportedExtensions(std::vector<const char*> extensions, std::vector<const char*>& unsupportedExtensions) const;
    inline std::vector<const char*> CheckPhysicalDeviceSupportedValidationLayers(VkPhysicalDevice physicalDevice,
    std::vector<const char*>                                                                      validationLayers,
    std::vector<const char*>&                                                                     unsupportedValidationLayers) const;
    inline std::vector<const char*> CheckPhysicalDeviceSupportedExtensions(VkPhysicalDevice physicalDevice, std::vector<const char*> extensions, std::vector<const char*>& unsupportedExtensions) const;
    inline VkResult                 GetPhysicalDeviceSupportSurfaceKHR(VkPhysicalDevice device, uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32* isSupported) const;
    inline VkResult                 GetInstanceVersion(uint32_t& major, uint32_t& minor, uint32_t& patch) const;

    // Overridden methods
    std::string GetAdapterDescription() const override;
    size_t      GetAdapterDedicatedVideoMemory() const override;

  protected:
    RIVulkanInstance                       VulkanInstance;
    CVulkanDevice                          VulkanDevice;
    std::vector<std::function<void(void)>> _cleanupResources;
};

}

#include "RICoreVk.inl"

#endif