#include "backend/vulkan/VulkanDevice13.h"

#include <gtest/gtest.h>

using namespace Fox;

VkAttachmentDescription
GetDefaultVkAttachmentDescription()
{
    VkAttachmentDescription desc;
    desc.format         = VK_FORMAT_UNDEFINED;
    desc.samples        = VK_SAMPLE_COUNT_1_BIT;
    desc.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    desc.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    desc.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    desc.initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    desc.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    return desc;
}

TEST(UnitVkAttachmentDescription, VkAttachmentDescriptionShouldHaveSameHashAndBeEqual)
{
    VkAttachmentDescription desc = GetDefaultVkAttachmentDescription();

    VkAttachmentDescription desc2 = GetDefaultVkAttachmentDescription();

    EXPECT_EQ(VkAttachmentDescriptionHashFn{}(desc), VkAttachmentDescriptionHashFn{}(desc2));
    EXPECT_TRUE(VkAttachmentDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkAttachmentDescriptionFormat, VkAttachmentDescriptionDifferentFormatShouldDifferHashAndCompare)
{
    VkAttachmentDescription desc = GetDefaultVkAttachmentDescription();

    VkAttachmentDescription desc2 = GetDefaultVkAttachmentDescription();
    desc.format                   = VK_FORMAT_R4G4_UNORM_PACK8;

    EXPECT_NE(VkAttachmentDescriptionHashFn{}(desc), VkAttachmentDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkAttachmentDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkAttachmentDescriptionSamples, VkAttachmentDescriptionDifferentSamplesShouldDifferHashAndCompare)
{
    VkAttachmentDescription desc = GetDefaultVkAttachmentDescription();

    VkAttachmentDescription desc2 = GetDefaultVkAttachmentDescription();
    desc.samples                  = VK_SAMPLE_COUNT_2_BIT;

    EXPECT_NE(VkAttachmentDescriptionHashFn{}(desc), VkAttachmentDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkAttachmentDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkAttachmentDescriptionLoadOp, VkAttachmentDescriptionDifferentLoadOpShouldDifferHashAndCompare)
{
    VkAttachmentDescription desc = GetDefaultVkAttachmentDescription();

    VkAttachmentDescription desc2 = GetDefaultVkAttachmentDescription();
    desc.loadOp                   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

    EXPECT_NE(VkAttachmentDescriptionHashFn{}(desc), VkAttachmentDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkAttachmentDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkAttachmentDescriptionStoreOp, VkAttachmentDescriptionDifferentStoreOpShouldDifferHashAndCompare)
{
    VkAttachmentDescription desc = GetDefaultVkAttachmentDescription();

    VkAttachmentDescription desc2 = GetDefaultVkAttachmentDescription();
    desc.storeOp                  = VK_ATTACHMENT_STORE_OP_STORE;

    EXPECT_NE(VkAttachmentDescriptionHashFn{}(desc), VkAttachmentDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkAttachmentDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkAttachmentDescriptionStencilLoadOp, VkAttachmentDescriptionDifferentLoadStoreOpShouldDifferHashAndCompare)
{
    VkAttachmentDescription desc = GetDefaultVkAttachmentDescription();

    VkAttachmentDescription desc2 = GetDefaultVkAttachmentDescription();
    desc.stencilLoadOp            = VK_ATTACHMENT_LOAD_OP_LOAD;

    EXPECT_NE(VkAttachmentDescriptionHashFn{}(desc), VkAttachmentDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkAttachmentDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkAttachmentDescriptionStencilStoreOp, VkAttachmentDescriptionDifferentStencilStoreOpShouldDifferHashAndCompare)
{
    VkAttachmentDescription desc = GetDefaultVkAttachmentDescription();

    VkAttachmentDescription desc2 = GetDefaultVkAttachmentDescription();
    desc.stencilStoreOp           = VK_ATTACHMENT_STORE_OP_STORE;

    EXPECT_NE(VkAttachmentDescriptionHashFn{}(desc), VkAttachmentDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkAttachmentDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkAttachmentDescriptionInitialLayout, VkAttachmentDescriptionDifferentInitialLayoutShouldDifferHashAndCompare)
{
    VkAttachmentDescription desc = GetDefaultVkAttachmentDescription();

    VkAttachmentDescription desc2 = GetDefaultVkAttachmentDescription();
    desc.initialLayout            = VK_IMAGE_LAYOUT_UNDEFINED;

    EXPECT_NE(VkAttachmentDescriptionHashFn{}(desc), VkAttachmentDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkAttachmentDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkAttachmentDescriptionFinalLayout, VkAttachmentDescriptionDifferentFinalLayoutShouldDifferHashAndCompare)
{
    VkAttachmentDescription desc = GetDefaultVkAttachmentDescription();

    VkAttachmentDescription desc2 = GetDefaultVkAttachmentDescription();
    desc.finalLayout              = VK_IMAGE_LAYOUT_UNDEFINED;

    EXPECT_NE(VkAttachmentDescriptionHashFn{}(desc), VkAttachmentDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkAttachmentDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitRenderPassCaching, EmptyRenderPassInfoShouldHaveSameHashAndBeEqual)
{
    RIRenderPassInfo       info;
    const RIRenderPassInfo expected;

    const auto infoHash     = RIRenderPassInfoHashFn{}(info);
    const auto expectedHash = RIRenderPassInfoHashFn{}(expected);
    EXPECT_EQ(infoHash, expectedHash);
    EXPECT_TRUE(RIRenderPassInfoEqualFn{}(info, expected));
}

TEST(UnitRenderPassCaching, SomeRenderPassInfoShouldHaveSameHashAndBeEqual)
{
    RIRenderPassInfo info;
    RIRenderPassInfo expected;
    {
        VkAttachmentDescription desc{};
        desc.format  = VK_FORMAT_UNDEFINED;
        desc.samples = VK_SAMPLE_COUNT_1_BIT;
        desc.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        // Stencil not used right now
        desc.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        desc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        desc.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        info.AttachmentDescription.emplace_back(desc);
        expected.AttachmentDescription.emplace_back(desc);
    }

    {
        VkAttachmentReference ref{};
        ref.attachment = 0;
        ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        info.ColorAttachmentReference.emplace_back(ref);
        expected.ColorAttachmentReference.emplace_back(ref);
    }
    {
        VkAttachmentReference ref{};
        ref.attachment = 0;
        ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        info.DepthStencilAttachmentReference.emplace_back(ref);
        expected.DepthStencilAttachmentReference.emplace_back(ref);
    }
    {
        VkSubpassDescription subpass{};
        subpass.flags                   = 0;
        subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.inputAttachmentCount    = 0;
        subpass.pInputAttachments       = nullptr;
        subpass.colorAttachmentCount    = (uint32_t)info.ColorAttachmentReference.size();
        subpass.pColorAttachments       = info.ColorAttachmentReference.data();
        subpass.pResolveAttachments     = NULL;
        subpass.pDepthStencilAttachment = (info.DepthStencilAttachmentReference.size()) ? info.DepthStencilAttachmentReference.data() : nullptr;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments    = NULL;

        info.SubpassDescription.emplace_back(subpass);
    }
    {
        VkSubpassDescription subpass{};
        subpass.flags                   = 0;
        subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.inputAttachmentCount    = 0;
        subpass.pInputAttachments       = nullptr;
        subpass.colorAttachmentCount    = (uint32_t)expected.ColorAttachmentReference.size();
        subpass.pColorAttachments       = expected.ColorAttachmentReference.data();
        subpass.pResolveAttachments     = NULL;
        subpass.pDepthStencilAttachment = (expected.DepthStencilAttachmentReference.size()) ? expected.DepthStencilAttachmentReference.data() : nullptr;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments    = NULL;

        expected.SubpassDescription.emplace_back(subpass);
    }

    {
        VkSubpassDependency dependency{};
        dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass    = 0;
        dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        info.SubpassDependency.emplace_back(dependency);
        expected.SubpassDependency.emplace_back(dependency);
    }

    const auto infoHash     = RIRenderPassInfoHashFn{}(info);
    const auto expectedHash = RIRenderPassInfoHashFn{}(expected);
    EXPECT_EQ(infoHash, expectedHash);
    const auto areEqual = RIRenderPassInfoEqualFn{}(info, expected);
    EXPECT_TRUE(areEqual);
}

TEST(UnitRenderPassCaching, SomeRenderPassInfoShouldNotHaveSameHashAndNotBeEqual)
{
    RIRenderPassInfo info;
    RIRenderPassInfo expected;
    
    const auto infoHash     = RIRenderPassInfoHashFn{}(info);
    const auto expectedHash = RIRenderPassInfoHashFn{}(expected);
    EXPECT_NE(infoHash, expectedHash);
    const auto areEqual = RIRenderPassInfoEqualFn{}(info, expected);
    EXPECT_FALSE(areEqual);
}