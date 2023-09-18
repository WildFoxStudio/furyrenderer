#pragma once

#include "backend/vulkan/ResourceTransfer.h"
#include "backend/vulkan/VulkanContext.h"
#include "backend/vulkan/VulkanContextFactory.h"
#include "backend/vulkan/VulkanDevice13.h"

static void
ExtractBuffer(Fox::VulkanContext* vkContext, Fox::DBuffer src, uint32_t bytes, void* outData)
{
    Fox::DBufferVulkan* vkSrcBuf = static_cast<Fox::DBufferVulkan*>(src);

    Fox::RIVulkanDevice13& device = vkContext->GetDevice();
    auto                   outBuf = device.CreateBufferHostVisible(bytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT);

    auto                   cmdPool   = device.CreateCommandPool();
    auto                   cmdBuffer = cmdPool->Allocate();
    Fox::CResourceTransfer transfer(cmdBuffer->Cmd);
    transfer.CopyVertexBuffer(vkSrcBuf->Buffer.Buffer, outBuf.Buffer, bytes, 0);
    transfer.FinishCommandBuffer();

    const std::vector<Fox::RICommandBuffer*> cmds{ cmdBuffer };
    device.SubmitToMainQueue(cmds, {}, VK_NULL_HANDLE, VK_NULL_HANDLE);
    vkContext->WaitDeviceIdle(); // wait copy operations to finish

    void* outputPtr = device.MapBuffer(outBuf);
    memcpy(outData, outputPtr, bytes);
    device.UnmapBuffer(outBuf);

    // cleanup
    device.DestroyBuffer(outBuf);
    device.DestroyCommandPool(cmdPool);
};