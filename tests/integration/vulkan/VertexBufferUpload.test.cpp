#include "WindowFixture.h"

#include "backend/vulkan/ResourceTransfer.h"
#include "backend/vulkan/VulkanContext.h"
#include "backend/vulkan/VulkanContextFactory.h"
#include "backend/vulkan/VulkanDevice13.h"

#include <array>

TEST_F(WindowFixture, ShouldCorrectlyUploadVertexBuffer)
{
    constexpr std::array<float, 6> ndcTriangle{ -1, -1, 3, -1, -1, 3 };
    constexpr size_t               bufSize = sizeof(float) * ndcTriangle.size();

    Fox::DContextConfig config;
    config.warningFunction = &WarningAssert;

    Fox::IContext* context = Fox::CreateVulkanContext(&config);
    ASSERT_NE(context, nullptr);

    Fox::DBuffer buf = context->CreateVertexBuffer(bufSize);

    ASSERT_NE(buf, nullptr);

    Fox::CopyDataCommand cmd;
    cmd.CopyVertex(buf, 0, (void*)ndcTriangle.data(), bufSize);

    context->SubmitCopy(std::move(cmd));

    context->AdvanceFrame();

    {
        Fox::VulkanContext* vkContext = static_cast<Fox::VulkanContext*>(context);
        vkContext->WaitDeviceIdle(); // wait copy operations to finish
        Fox::DBufferVulkan* vkBuffer = static_cast<Fox::DBufferVulkan*>(buf);

        Fox::RIVulkanDevice13& device = vkContext->GetDevice();
        auto                   outBuf = device.CreateBufferHostVisible(bufSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        auto                   cmdPool   = device.CreateCommandPool();
        auto                   cmdBuffer = cmdPool->Allocate();
        Fox::CResourceTransfer transfer(cmdBuffer->Cmd);
        transfer.CopyVertexBuffer(vkBuffer->Buffer.Buffer, outBuf.Buffer, bufSize, 0);
        transfer.FinishCommandBuffer();

        const std::vector<Fox::RICommandBuffer*> cmds{ cmdBuffer };
        device.SubmitToMainQueue(cmds, {}, VK_NULL_HANDLE, VK_NULL_HANDLE);
        vkContext->WaitDeviceIdle(); // wait copy operations to finish

        void*                outputPtr = device.MapBuffer(outBuf);
        std::array<float, 6> expected;
        memcpy(expected.data(), outputPtr, bufSize);
        device.UnmapBuffer(outBuf);
        device.DestroyBuffer(outBuf);
        device.DestroyCommandPool(cmdPool);

        EXPECT_EQ(ndcTriangle, expected);
    }

    context->DestroyVertexBuffer(buf);

    delete context;
}
