#include "WindowFixture.h"

#include "ExtractImage.h"
#include "backend/vulkan/ResourceTransfer.h"
#include "backend/vulkan/VulkanContext.h"
#include "backend/vulkan/VulkanContextFactory.h"
#include "backend/vulkan/VulkanDevice13.h"

#include <array>

TEST_F(WindowFixture, ShouldCorrectlyUploadImage)
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

    uint32_t img = context->CreateImage(Fox::EFormat::R8G8B8A8_UNORM, 2, 2, 1);

    //{
    //    uint32_t   stagingBuf    = context->CreateBuffer(data.size(), Fox::EResourceType::TRANSFER, Fox::EMemoryUsage::RESOURCE_MEMORY_USAGE_CPU_TO_GPU);
    //    const auto transferQueue = context->FindQueue(Fox::EQueueType::QUEUE_TRANSFER);

    //    const auto pool = context->CreateCommandPool(transferQueue);
    //    const auto cmd  = context->CreateCommandBuffer(pool);
    //    context->BeginCommandBuffer(cmd);
    //    Fox::TextureBarrier barrier;
    //    barrier.ImageId      = img;
    //    barrier.CurrentState = Fox::EResourceState::UNDEFINED;
    //    barrier.NewState     = Fox::EResourceState::COPY_DEST;
    //    context->ResourceBarrier(cmd, 0, nullptr, 1, &barrier, 0, nullptr);
    //    context->CopyImage(cmd, img, 2, 2, 0, stagingBuf, 0);
    //    context->EndCommandBuffer(cmd);

    //    const auto fence = context->CreateFence(false);
    //    context->QueueSubmit({}, {}, { cmd }, fence);
    //    context->WaitForFence(fence, 0xFFFFFFFF);
    //    context->DestroyFence(fence);
    //    context->DestroyCommandBuffer(cmd);
    //    context->DestroyCommandPool(pool);
    //    context->DestroyBuffer(stagingBuf);
    //}

    //ASSERT_NE(img, 0);

    //{
    //    Fox::VulkanContext*           vkContext = static_cast<Fox::VulkanContext*>(context);
    //    std::array<unsigned char, 16> expected;
    //    ExtractImage(vkContext, img, 2, 2, 1, VK_FORMAT_R8G8B8A8_UNORM, data.size(), (void*)expected.data());

    //    EXPECT_EQ(data, expected);
    //}

    context->DestroyImage(img);

    delete context;
}
