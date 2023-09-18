#pragma once

#include "backend/vulkan/ResourceTransfer.h"
#include "backend/vulkan/VulkanContext.h"
#include "backend/vulkan/VulkanContextFactory.h"
#include "backend/vulkan/VulkanDevice13.h"

static void
ExtractImage(Fox::VulkanContext* vkContext, Fox::DImage src, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, uint32_t bytes, void* outData)
{
    Fox::DImageVulkan* vkSrcImg = static_cast<Fox::DImageVulkan*>(src);

    Fox::RIVulkanDevice13& device = vkContext->GetDevice();

    auto                   cmdPool   = device.CreateCommandPool();
    auto                   cmdBuffer = cmdPool->Allocate();
    Fox::CResourceTransfer transfer(cmdBuffer->Cmd);

    auto outImg = device.CreateBufferHostVisible(16, VK_IMAGE_USAGE_TRANSFER_DST_BIT);

    transfer.CopyImageToBuffer(outImg.Buffer, vkSrcImg->Image.Image, { width, height }, 0);

    transfer.FinishCommandBuffer();

    const std::vector<Fox::RICommandBuffer*> cmds{ cmdBuffer };
    device.SubmitToMainQueue(cmds, {}, VK_NULL_HANDLE, VK_NULL_HANDLE);
    vkContext->WaitDeviceIdle(); // wait copy operations to finish

    void* outputPtr = device.MapBuffer(outImg);
    memcpy(outData, outputPtr, bytes);
    device.UnmapBuffer(outImg);

    //// cleanup
    device.DestroyBuffer(outImg);
    device.DestroyCommandPool(cmdPool);
};