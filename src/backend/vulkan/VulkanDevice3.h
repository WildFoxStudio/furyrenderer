// Copyright RedFox Studio 2022

#pragma once

#include "VulkanDevice2.h"

#include <volk.h>

#include <unordered_set>

namespace Fox
{
// synchronization primitives implementation
class RIVulkanDevice3 : public RIVulkanDevice2
{

  public:
    virtual ~RIVulkanDevice3();
    VkSemaphore CreateVkSemaphore();
    void        DestroyVkSemaphore(VkSemaphore semaphore);

    VkFence CreateFence(bool signaled);
    void    DestroyFence(VkFence fence);

  private:
    std::unordered_set<VkFence>     _fences;
    std::unordered_set<VkSemaphore> _semaphores;
};
}