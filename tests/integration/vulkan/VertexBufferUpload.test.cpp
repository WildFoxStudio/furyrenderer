#include "WindowFixture.h"

#include "ExtractBuffer.h"

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
    config.warningFunction   = &WarningAssert;
    config.stagingBufferSize = bufSize;

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

        std::array<float, 6> expected;
        ExtractBuffer(vkContext, buf, bufSize, expected.data());

        EXPECT_EQ(ndcTriangle, expected);
    }

    context->DestroyVertexBuffer(buf);

    delete context;
}

TEST_F(WindowFixture, ShouldCorrectlyUploadMultipleVertexBuffer)
{

    constexpr std::array<float, 6> ndcTriangle{ -1, -1, 3, -1, -1, 3 };
    constexpr size_t               triangleSize = sizeof(float) * ndcTriangle.size();

    constexpr std::array<float, 10> data{ 1, 2, 3, 4, 5, 5, 6, 7, 8, 9 };
    constexpr size_t                dataSize = sizeof(float) * data.size();

    constexpr std::array<float, 4> data2{ 1, 2, 3, 4 };
    constexpr size_t               data2Size = sizeof(float) * data2.size();

    Fox::DContextConfig config;
    config.warningFunction   = &WarningAssert;
    config.stagingBufferSize = triangleSize + dataSize + data2Size;

    Fox::IContext* context = Fox::CreateVulkanContext(&config);
    ASSERT_NE(context, nullptr);

    Fox::DBuffer buf = context->CreateVertexBuffer(triangleSize);
    ASSERT_NE(buf, nullptr);
    Fox::CopyDataCommand cmd;
    cmd.CopyVertex(buf, 0, (void*)ndcTriangle.data(), triangleSize);
    context->SubmitCopy(std::move(cmd));

    Fox::DBuffer buf1 = context->CreateVertexBuffer(dataSize);
    ASSERT_NE(buf1, nullptr);
    Fox::CopyDataCommand cmd1;
    cmd1.CopyVertex(buf1, 0, (void*)data.data(), dataSize);
    context->SubmitCopy(std::move(cmd1));

    Fox::DBuffer buf2 = context->CreateVertexBuffer(data2Size);
    ASSERT_NE(buf2, nullptr);
    Fox::CopyDataCommand cmd2;
    cmd2.CopyVertex(buf2, 0, (void*)data2.data(), data2Size);
    context->SubmitCopy(std::move(cmd2));

    context->AdvanceFrame();

    {
        Fox::VulkanContext* vkContext = static_cast<Fox::VulkanContext*>(context);
        vkContext->WaitDeviceIdle(); // wait copy operations to finish

        {
            std::array<float, 6> output;
            ExtractBuffer(vkContext, buf, triangleSize, output.data());
            EXPECT_EQ(ndcTriangle, output);
        }
        {
            std::array<float, 10> output;
            ExtractBuffer(vkContext, buf1, dataSize, output.data());
            EXPECT_EQ(data, output);
        }
        {
            std::array<float, 4> output;
            ExtractBuffer(vkContext, buf2, data2Size, output.data());
            EXPECT_EQ(data2, output);
        }
    }

    context->DestroyVertexBuffer(buf);
    context->DestroyVertexBuffer(buf1);
    context->DestroyVertexBuffer(buf2);

    delete context;
}
