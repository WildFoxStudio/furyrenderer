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

    Fox::DImage img = context->CreateImage(Fox::EFormat::R8G8B8A8_UNORM, 2, 2, 1);

    ASSERT_NE(img, nullptr);

    Fox::CopyDataCommand cmd;
    cmd.CopyImageMipMap(img, 0, (void*)data.data(), 2, 2, 0, data.size());

    context->SubmitCopy(std::move(cmd));

    context->AdvanceFrame();

    {
        Fox::VulkanContext* vkContext = static_cast<Fox::VulkanContext*>(context);
        vkContext->WaitDeviceIdle(); // wait copy operations to finish

        std::array<unsigned char, 16> expected;
        ExtractImage(vkContext, img, 2, 2, 1, VK_FORMAT_R8G8B8A8_UNORM, data.size(), (void*)expected.data());

        EXPECT_EQ(data, expected);
    }

    context->DestroyImage(img);

    delete context;
}
