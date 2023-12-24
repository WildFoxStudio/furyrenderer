// Copyright RedFox Studio 2022

#pragma once

#define VK_NO_PROTOTYPES
#define VMA_VULKAN_VERSION 1002000 // Vulkan 1.2
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1 // define 1 for custom func ptr from volk
#define VMA_STATIC_VULKAN_FUNCTIONS 0 // define 0 for custom func ptr from volk
#include "vk_mem_alloc.h"

#include <vulkan/vulkan.h>

#include "VulkanInstance.h"

#include <vector>

namespace Fox
{

struct RIVulkanQueue
{
    uint32_t Flags{};
    uint32_t FamilyIndex{};
    uint32_t QueueIndex{};
};

class RIVulkanDevice
{
  public:
    virtual ~RIVulkanDevice();
    VkResult Create(const RIVulkanInstance& instance,
    const void*                             pNext,
    VkPhysicalDevice                        hardwareDevice,
    std::vector<const char*>                extensions,
    VkPhysicalDeviceFeatures*               optDeviceFeatures,
    std::vector<const char*>                validationLayers);
    void     Deinit();

    RIVulkanQueue GraphicsQueueInfo{};
    RIVulkanQueue TransferQueueInfo{};
    std::vector<VkQueueFamilyProperties> QueueFamilies;

    inline int32_t GetMaxImageAllocations() const { return 4096; }

    VkPhysicalDevice                 PhysicalDevice{}; /* GPU chosen as the default device */
    VkDevice                         Device{}; /* Logical device */
    VmaAllocator                     VmaAllocator;
    VkPhysicalDeviceProperties       DeviceProperties{}; /*Properties of the physical device*/
    VkPhysicalDeviceMemoryProperties DeviceMemory{}; /*Properties about the physical device memory*/
    VkQueue                          MainQueue{}; /* Graphics and Transfer queue*/
    VkQueue                          TransferQueue{}; /* Transfer queue*/

  private:

    uint32_t _queryGraphicsAndTransferQueueIndex() const;
};

}