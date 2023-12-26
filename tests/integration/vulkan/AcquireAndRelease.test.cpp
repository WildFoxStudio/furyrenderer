#include "WindowFixture.h"

#include "ExtractImage.h"
#include "backend/vulkan/ResourceTransfer.h"
#include "backend/vulkan/VulkanContext.h"
#include "backend/vulkan/VulkanContextFactory.h"
#include "backend/vulkan/VulkanDevice13.h"

#include <array>

TEST_F(WindowFixture, ShouldCorrectlyReleaseAndAcquireResources)
{
    constexpr std::array<unsigned char, 4 * 4> data{
        0xff,
        0,
        0,
        0xff,
        0,
        0xff,
        0,
        0xff,
        0xff,
        0,
        0,
        0xff,
        0,
        0xff,
        0,
        0xff,
    };

    Fox::DContextConfig config;
    config.warningFunction   = &WarningAssert;
    config.stagingBufferSize = 10 * 1024 * 1024; // 10mb

    Fox::IContext* context = Fox::CreateVulkanContext(&config);
    ASSERT_NE(context, nullptr);
    const auto graphicsQueue = context->FindQueue(Fox::EQueueType::QUEUE_GRAPHICS);
    const auto transferQueue = context->FindQueue(Fox::EQueueType::QUEUE_TRANSFER);
    ASSERT_NE(graphicsQueue, transferQueue);

    uint32_t img = context->CreateImage(Fox::EFormat::R8G8B8A8_UNORM, 2, 2, 1);
    uint32_t buf = context->CreateBuffer(12, Fox::EResourceType::VERTEX_INDEX_BUFFER, Fox::EMemoryUsage::RESOURCE_MEMORY_USAGE_GPU_ONLY);

    {
        uint32_t stagingBuf = context->CreateBuffer(data.size(), Fox::EResourceType::TRANSFER, Fox::EMemoryUsage::RESOURCE_MEMORY_USAGE_CPU_TO_GPU);

        // Upload something to img and buf - so the ownership is acquired for the transfer queue
        {
            const auto pool = context->CreateCommandPool(transferQueue);
            const auto cmd  = context->CreateCommandBuffer(pool);

            context->BeginCommandBuffer(cmd);
            {
                Fox::TextureBarrier barrier{};
                barrier.ImageId      = img;
                barrier.CurrentState = Fox::EResourceState::UNDEFINED;
                barrier.NewState     = Fox::EResourceState::COPY_DEST;

                Fox::BufferBarrier bufBarrier{};
                bufBarrier.BufferId     = buf;
                bufBarrier.CurrentState = Fox::EResourceState::UNDEFINED;
                bufBarrier.NewState     = Fox::EResourceState::COPY_DEST;
                context->ResourceBarrier(cmd, 1, &bufBarrier, 1, &barrier, 0, nullptr);

                context->CopyImage(cmd, img, 2, 2, 0, stagingBuf, 0);
            }
            {
                // Release ownership
                Fox::TextureBarrier barrier{};
                barrier.ImageId           = img;
                barrier.CurrentState      = Fox::EResourceState::COPY_DEST;
                barrier.NewState          = Fox::EResourceState::PIXEL_SHADER_RESOURCE;
                barrier.TransferOwnership = Fox::ETransferOwnership::RELEASE;
                barrier.SrcQueue          = transferQueue;
                barrier.DstQueue          = graphicsQueue;

                Fox::BufferBarrier bufBarrier{};
                bufBarrier.BufferId          = buf;
                bufBarrier.CurrentState      = Fox::EResourceState::COPY_DEST;
                bufBarrier.NewState          = Fox::EResourceState::VERTEX_AND_CONSTANT_BUFFER;
                bufBarrier.TransferOwnership = Fox::ETransferOwnership::RELEASE;
                bufBarrier.SrcQueue          = transferQueue;
                bufBarrier.DstQueue          = graphicsQueue;
                context->ResourceBarrier(cmd, 1, &bufBarrier, 1, &barrier, 0, nullptr);
            }

            context->EndCommandBuffer(cmd);

            const auto fence = context->CreateFence(false);
            context->QueueSubmit(transferQueue, {}, {}, { cmd }, fence);
            context->WaitForFence(fence, 0xFFFFFFFF);
            context->DestroyCommandBuffer(cmd);
            context->DestroyCommandPool(pool);
            context->DestroyFence(fence);
        }

        // Acquire on the graphics queue
        {
            const auto pool = context->CreateCommandPool(graphicsQueue);
            const auto cmd  = context->CreateCommandBuffer(pool);
            context->BeginCommandBuffer(cmd);

            Fox::TextureBarrier imgBarrier{};
            imgBarrier.ImageId           = img;
            imgBarrier.CurrentState      = Fox::EResourceState::COPY_DEST;
            imgBarrier.NewState          = Fox::EResourceState::PIXEL_SHADER_RESOURCE;
            imgBarrier.TransferOwnership = Fox::ACQUIRE;
            imgBarrier.SrcQueue          = transferQueue;
            imgBarrier.DstQueue          = graphicsQueue;

            Fox::BufferBarrier bufBarrier{};
            bufBarrier.BufferId          = buf;
            bufBarrier.CurrentState      = Fox::EResourceState::COPY_DEST;
            bufBarrier.NewState          = Fox::EResourceState::VERTEX_AND_CONSTANT_BUFFER;
            bufBarrier.TransferOwnership = Fox::ACQUIRE;
            bufBarrier.SrcQueue          = transferQueue;
            bufBarrier.DstQueue          = graphicsQueue;

            context->ResourceBarrier(cmd, 1, &bufBarrier, 1, &imgBarrier, 0, nullptr);
            context->EndCommandBuffer(cmd);

            const auto fence = context->CreateFence(false);
            context->QueueSubmit(graphicsQueue, {}, {}, { cmd }, fence);
            context->WaitForFence(fence, 0xFFFFFFFF);

            context->DestroyCommandBuffer(cmd);
            context->DestroyCommandPool(pool);
            context->DestroyFence(fence);
        }

        context->DestroyBuffer(stagingBuf);
    }

    context->DestroyBuffer(buf);
    context->DestroyImage(img);

    delete context;
}
