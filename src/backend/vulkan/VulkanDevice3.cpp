// Copyright RedFox Studio 2022

#include "VulkanDevice3.h"

#include "Core/utils/debug.h"
#include "UtilsVK.h"

namespace Fox
{

RIVulkanDevice3::~RIVulkanDevice3()
{
    check(_fences.size() == 0);
    check(_semaphores.size() == 0);
}

VkSemaphore
RIVulkanDevice3::CreateVkSemaphore()
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreInfo.flags = VK_SEMAPHORE_TYPE_BINARY;

    VkSemaphore    semaphore{};
    const VkResult result = vkCreateSemaphore(Device, &semaphoreInfo, nullptr, &semaphore);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkErrorString(result));
        }
    _semaphores.insert(semaphore);

    return semaphore;
}

void
RIVulkanDevice3::DestroyVkSemaphore(VkSemaphore semaphore)
{
    vkDestroySemaphore(Device, semaphore, nullptr);
    _semaphores.erase(_semaphores.find(semaphore));
}

VkFence
RIVulkanDevice3::CreateFence(bool signaled)
{
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

    VkFence        fence{};
    const VkResult result = vkCreateFence(Device, &fenceInfo, nullptr, &fence);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkErrorString(result));
        }

    _fences.insert(fence);

    return fence;
}

void
RIVulkanDevice3::DestroyFence(VkFence fence)
{
    vkDestroyFence(Device, fence, nullptr);
    _fences.erase(_fences.find(fence));
}

}