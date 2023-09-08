// Copyright RedFox Studio 2022

#include "VulkanDevice.h"
#include "Core/utils/debug.h"
#include "UtilsVK.h"

#define VMA_IMPLEMENTATION
#include "VulkanMemoryAllocator/include/vk_mem_alloc.h"

#include <volk.h>

#include <iostream>

namespace Fox
{

RIVulkanDevice::~RIVulkanDevice()
{
    check(PhysicalDevice == nullptr);
    check(Device == nullptr);
};

bool
RIVulkanDevice::Create(const RIVulkanInstance& instance,
VkPhysicalDevice                               hardwareDevice,
std::vector<const char*>                       extensions,
VkPhysicalDeviceFeatures&                      deviceFeatures,
std::vector<const char*>                       validationLayers)
{
    check(PhysicalDevice == nullptr);
    check(Device == nullptr);

    PhysicalDevice = hardwareDevice;

    _queueFamilyIndex = _queryGraphicsAndTransferQueueIndex();

    // Get it's physical properties
    vkGetPhysicalDeviceProperties(PhysicalDevice, &DeviceProperties);

    // It's memory properties
    vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &DeviceMemory);

    const float queuePriority = 1.f;

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = _queueFamilyIndex;
    queueCreateInfo.queueCount       = 1;
    queueCreateInfo.flags            = 0;
    queueCreateInfo.pNext            = NULL;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.enabledExtensionCount   = 0;
    deviceInfo.pQueueCreateInfos       = &queueCreateInfo;
    deviceInfo.queueCreateInfoCount    = 1;
    deviceInfo.pEnabledFeatures        = &deviceFeatures;
    deviceInfo.enabledExtensionCount   = (uint32_t)extensions.size();
    deviceInfo.ppEnabledExtensionNames = extensions.data();
    deviceInfo.pNext                   = NULL;
    deviceInfo.enabledLayerCount       = (uint32_t)validationLayers.size();
    deviceInfo.ppEnabledLayerNames     = validationLayers.data();

    // Create the device
    const VkResult result = vkCreateDevice(PhysicalDevice, &deviceInfo, nullptr, &Device);
    if (VKFAILED(result))
        {
            // std::runtime_error("Failed to initialize VkDevice" + VKERRORSTRING(result));
            return false;
        }
    {
        VmaVulkanFunctions vma_vulkan_func{};
        vma_vulkan_func.vkGetInstanceProcAddr                   = vkGetInstanceProcAddr;
        vma_vulkan_func.vkGetDeviceProcAddr                     = vkGetDeviceProcAddr;
        vma_vulkan_func.vkAllocateMemory                        = vkAllocateMemory;
        vma_vulkan_func.vkBindBufferMemory                      = vkBindBufferMemory;
        vma_vulkan_func.vkBindImageMemory                       = vkBindImageMemory;
        vma_vulkan_func.vkCreateBuffer                          = vkCreateBuffer;
        vma_vulkan_func.vkCreateImage                           = vkCreateImage;
        vma_vulkan_func.vkDestroyBuffer                         = vkDestroyBuffer;
        vma_vulkan_func.vkDestroyImage                          = vkDestroyImage;
        vma_vulkan_func.vkFlushMappedMemoryRanges               = vkFlushMappedMemoryRanges;
        vma_vulkan_func.vkFreeMemory                            = vkFreeMemory;
        vma_vulkan_func.vkGetBufferMemoryRequirements           = vkGetBufferMemoryRequirements;
        vma_vulkan_func.vkGetImageMemoryRequirements            = vkGetImageMemoryRequirements;
        vma_vulkan_func.vkGetPhysicalDeviceMemoryProperties     = vkGetPhysicalDeviceMemoryProperties;
        vma_vulkan_func.vkGetPhysicalDeviceProperties           = vkGetPhysicalDeviceProperties;
        vma_vulkan_func.vkInvalidateMappedMemoryRanges          = vkInvalidateMappedMemoryRanges;
        vma_vulkan_func.vkMapMemory                             = vkMapMemory;
        vma_vulkan_func.vkUnmapMemory                           = vkUnmapMemory;
        vma_vulkan_func.vkCmdCopyBuffer                         = vkCmdCopyBuffer;
        vma_vulkan_func.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR;
        vma_vulkan_func.vkGetBufferMemoryRequirements2KHR       = vkGetBufferMemoryRequirements2KHR;
        vma_vulkan_func.vkGetImageMemoryRequirements2KHR        = vkGetImageMemoryRequirements2KHR;
        vma_vulkan_func.vkBindBufferMemory2KHR                  = vkBindBufferMemory2KHR;
        vma_vulkan_func.vkBindImageMemory2KHR                   = vkBindImageMemory2KHR;

        check(vma_vulkan_func.vkGetPhysicalDeviceMemoryProperties2KHR);
        check(vma_vulkan_func.vkGetBufferMemoryRequirements2KHR);
        check(vma_vulkan_func.vkGetImageMemoryRequirements2KHR);
        check(vma_vulkan_func.vkBindBufferMemory2KHR);
        check(vma_vulkan_func.vkBindImageMemory2KHR);

        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.vulkanApiVersion       = VK_API_VERSION_1_1;
        allocatorCreateInfo.physicalDevice         = PhysicalDevice;
        allocatorCreateInfo.device                 = Device;
        allocatorCreateInfo.instance               = instance.Instance;
        allocatorCreateInfo.pVulkanFunctions       = &vma_vulkan_func;

        const VkResult result = vmaCreateAllocator(&allocatorCreateInfo, &VmaAllocator);
        if (result != VK_SUCCESS)
            {
                // std::runtime_error("Failed to initialize VMA");
                return false;
            }
    }

    vkGetDeviceQueue(Device, _queueFamilyIndex, 0, &MainQueue);

    return true;
}

void
RIVulkanDevice::Deinit()
{
    check(PhysicalDevice != nullptr);
    check(Device != nullptr);

    vkDestroyDevice(Device, nullptr);

    Device         = nullptr;
    PhysicalDevice = nullptr;
}

uint32_t
RIVulkanDevice::_queryGraphicsAndTransferQueueIndex() const
{
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &queueFamilyCount, queueFamilyProperties.data());
    critical(queueFamilyProperties.size() > 0);
    if (queueFamilyProperties.size() == 0)
        {
            throw std::runtime_error("No VkQueueFamilyProperties available\n");
        }

    auto it = std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(), [](const VkQueueFamilyProperties& p) {
        constexpr int32_t FLAGS = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT;
        return p.queueCount > 0 && p.queueFlags & FLAGS;
    });

    if (it == queueFamilyProperties.end())
        {
            throw std::runtime_error("No graphics queue is available\n");
        }

    auto graphicsAndTransferQueueFamilyIndex{ std::distance(queueFamilyProperties.begin(), it) };

    std::cout << "Graphics and Present queue family index:" << graphicsAndTransferQueueFamilyIndex << "\n";

    return graphicsAndTransferQueueFamilyIndex;
}
}