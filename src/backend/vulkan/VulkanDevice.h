// Copyright RedFox Studio 2022

#pragma once

#define VK_NO_PROTOTYPES
#define VMA_VULKAN_VERSION 1002000 // Vulkan 1.2
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1 // define 1 for custom func ptr from volk
#define VMA_STATIC_VULKAN_FUNCTIONS 0 // define 0 for custom func ptr from volk
#include "VulkanMemoryAllocator/include/vk_mem_alloc.h"

#include <vulkan/vulkan.h>

#include "VulkanInstance.h"

namespace Fox
{

class RIVulkanDevice
{
  public:
    virtual ~RIVulkanDevice();
    bool Create(const RIVulkanInstance& instance,
    VkPhysicalDevice                    hardwareDevice,
    std::vector<const char*>            extensions,
    VkPhysicalDeviceFeatures&           deviceFeatures,
    std::vector<const char*>            validationLayers);
    void Deinit();

    inline int32_t GetQueueFamilyIndex() const { return _queueFamilyIndex; };

    inline int32_t GetMaxImageAllocations() const { return 4096; }

    VkPhysicalDevice                 PhysicalDevice{}; /* GPU chosen as the default device */
    VkDevice                         Device{}; /* Logical device */
    VmaAllocator                     VmaAllocator;
    VkPhysicalDeviceProperties       DeviceProperties{}; /*Properties of the physical device*/
    VkPhysicalDeviceMemoryProperties DeviceMemory{}; /*Properties about the physical device memory*/
    VkQueue                          MainQueue{}; /* Graphics and Transfer queue*/
  private:
    uint32_t _queueFamilyIndex;
    uint32_t _queryGraphicsAndTransferQueueIndex() const;
};

}