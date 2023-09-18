#include "WindowFixture.h"

#include "ExtractBuffer.h"

#include "backend/vulkan/ResourceTransfer.h"
#include "backend/vulkan/VulkanContext.h"
#include "backend/vulkan/VulkanContextFactory.h"
#include "backend/vulkan/VulkanDevice13.h"

#include <array>


TEST_F(WindowFixture, ShouldCorrectlyUploadUniformBuffer)
{
    constexpr std::array<float, 6> data{ -1, -1, 3, -1, -1, 3 };
    constexpr size_t               bufSize = sizeof(float) * data.size();

    Fox::DContextConfig config;
    config.warningFunction   = &WarningAssert;
    config.stagingBufferSize = bufSize;

    Fox::IContext* context = Fox::CreateVulkanContext(&config);
    ASSERT_NE(context, nullptr);

    Fox::DBuffer buf = context->CreateUniformBuffer(bufSize);

    ASSERT_NE(buf, nullptr);

    Fox::CopyDataCommand cmd;
    cmd.CopyUniformBuffer(buf, (void*)data.data(), bufSize);

    context->SubmitCopy(std::move(cmd));

    context->AdvanceFrame();

    {
        Fox::VulkanContext* vkContext = static_cast<Fox::VulkanContext*>(context);
        vkContext->WaitDeviceIdle(); // wait copy operations to finish

        std::array<float, 6> expected;
        ExtractBuffer(vkContext, buf, bufSize, expected.data());

        EXPECT_EQ(data, expected);
    }

    context->DestroyUniformBuffer(buf);

    delete context;
}
