// Copyright RedFox Studio 2022

#include "VulkanDevice12.h"

#include "asserts.h"

#include "UtilsVK.h"

#include <algorithm>

namespace Fox
{

RIVulkanDevice12::~RIVulkanDevice12() { check(_cachedPools.size() == 0); }

RICommandBuffer*
RICommandPool::Allocate()
{
    VkCommandBufferAllocateInfo info{};
    info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.pNext              = NULL;
    info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandPool        = _poolManager;
    info.commandBufferCount = (uint32_t)1;

    VkCommandBuffer cmd{};
    const VkResult  result = vkAllocateCommandBuffers(_device, &info, &cmd);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkUtils::VkErrorString(result));
        }

    _cachedCmds.push_back(RICommandBuffer(cmd));

    return &_cachedCmds.back();
}

void
RICommandPool::Reset()
{
    check(_poolManager); // has beed moved, invalid state

    vkResetCommandPool(_device, _poolManager, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
    std::for_each(_cachedCmds.begin(), _cachedCmds.end(), [](RICommandBuffer& cmd) {
        if (cmd.Count() > 0)
            {
                cmd.DecreaseCounter();
            }
        check(cmd.Count() == 0);
    });
}

RICommandPool*
RIVulkanDevice12::CreateCommandPool(uint32_t queueFamilyIndex)
{
    VkCommandPool commandPool{};
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; /*Allow command buffers to be rerecorded individually, without this flag they all have to be reset together*/
        poolInfo.queueFamilyIndex = queueFamilyIndex;

        const VkResult result = vkCreateCommandPool(Device, &poolInfo, nullptr, &commandPool);
        if (VKFAILED(result))
            {
                throw std::runtime_error(VkUtils::VkErrorString(result));
            }
    }

    _cachedPools.push_back(RICommandPool(Device, commandPool));

    return &_cachedPools.back();
}

void
RIVulkanDevice12::DestroyCommandPool(RICommandPool* pool)
{
    vkDestroyCommandPool(Device, pool->_poolManager, nullptr);

    // remove from the list
    for (auto it = _cachedPools.begin(); it != _cachedPools.end(); ++it)
        {
            if (&(*it) == pool)
                {
                    _cachedPools.erase(it);
                    break;
                }
        }
}

VkCommandPool
RIVulkanDevice12::CreateCommandPool2(uint32_t queueFamilyIndex)
{
    VkCommandPool commandPool{};
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags            = VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT; /* they all have to be reset together*/
        poolInfo.queueFamilyIndex = queueFamilyIndex;

        const VkResult result = vkCreateCommandPool(Device, &poolInfo, nullptr, &commandPool);
        if (VKFAILED(result))
            {
                throw std::runtime_error(VkUtils::VkErrorString(result));
            }
    }

    _cachedPools2.push_back(commandPool);

    return commandPool;
}

void
RIVulkanDevice12::ResetCommandPool2(VkCommandPool commandPool)
{
    vkResetCommandPool(Device, commandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
}

void
RIVulkanDevice12::DestroyCommandPool2(VkCommandPool commandPool)
{
    vkDestroyCommandPool(Device, commandPool, nullptr);

    // remove from the list
    for (auto it = _cachedPools2.begin(); it != _cachedPools2.end(); ++it)
        {
            if ((*it) == commandPool)
                {
                    _cachedPools2.erase(it);
                    break;
                }
        }
}

void
RIVulkanDevice12::SubmitToMainQueue(VkQueue queue, const std::vector<RICommandBuffer*>& cmds, const std::vector<VkSemaphore>& waitSemaphore, VkSemaphore finishSemaphore, VkFence fence)
{

    std::for_each(cmds.begin(), cmds.end(), [](RICommandBuffer* cmd) { cmd->IncreaseCounter(); });

    const std::vector<VkPipelineStageFlags> waitStages(waitSemaphore.size(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

    std::vector<VkCommandBuffer> commandBuffers(cmds.size());

    std::transform(cmds.begin(), cmds.end(), commandBuffers.begin(), [](const RICommandBuffer* cmd) { return cmd->Cmd; });

    check(commandBuffers.size() == cmds.size());

    VkSubmitInfo submitInfo{};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = (uint32_t)waitSemaphore.size();
    submitInfo.pWaitSemaphores      = waitSemaphore.data();
    submitInfo.pWaitDstStageMask    = waitStages.data();
    submitInfo.commandBufferCount   = (uint32_t)commandBuffers.size();
    submitInfo.pCommandBuffers      = commandBuffers.data();
    submitInfo.signalSemaphoreCount = finishSemaphore ? 1 : 0;
    submitInfo.pSignalSemaphores    = finishSemaphore ? &finishSemaphore : nullptr;

    const VkResult result = vkQueueSubmit(queue, 1, &submitInfo, fence);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkUtils::VkErrorString(result));
        }
}

}