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

VkSubpassDependency
GetDefaultVkSubpassDependency()
{
    VkSubpassDependency dependency;
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    return dependency;
}

VkSubpassDescription
GetDefaultVkSubpassDescription(const std::vector<VkAttachmentReference>& colorAttachment, const std::vector<VkAttachmentReference>& depthStencilAttachment)
{
    VkSubpassDescription subpass{};
    subpass.flags                   = 0;
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.inputAttachmentCount    = 0;
    subpass.pInputAttachments       = nullptr;
    subpass.colorAttachmentCount    = (uint32_t)colorAttachment.size();
    subpass.pColorAttachments       = colorAttachment.data();
    subpass.pResolveAttachments     = NULL;
    subpass.pDepthStencilAttachment = (depthStencilAttachment.size()) ? depthStencilAttachment.data() : nullptr;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments    = NULL;
    return subpass;
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
    desc2.format                  = VK_FORMAT_R4G4_UNORM_PACK8;

    EXPECT_NE(VkAttachmentDescriptionHashFn{}(desc), VkAttachmentDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkAttachmentDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkAttachmentDescriptionSamples, VkAttachmentDescriptionDifferentSamplesShouldDifferHashAndCompare)
{
    VkAttachmentDescription desc = GetDefaultVkAttachmentDescription();

    VkAttachmentDescription desc2 = GetDefaultVkAttachmentDescription();
    desc2.samples                 = VK_SAMPLE_COUNT_2_BIT;

    EXPECT_NE(VkAttachmentDescriptionHashFn{}(desc), VkAttachmentDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkAttachmentDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkAttachmentDescriptionLoadOp, VkAttachmentDescriptionDifferentLoadOpShouldDifferHashAndCompare)
{
    VkAttachmentDescription desc = GetDefaultVkAttachmentDescription();

    VkAttachmentDescription desc2 = GetDefaultVkAttachmentDescription();
    desc2.loadOp                  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

    EXPECT_NE(VkAttachmentDescriptionHashFn{}(desc), VkAttachmentDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkAttachmentDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkAttachmentDescriptionStoreOp, VkAttachmentDescriptionDifferentStoreOpShouldDifferHashAndCompare)
{
    VkAttachmentDescription desc = GetDefaultVkAttachmentDescription();

    VkAttachmentDescription desc2 = GetDefaultVkAttachmentDescription();
    desc2.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;

    EXPECT_NE(VkAttachmentDescriptionHashFn{}(desc), VkAttachmentDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkAttachmentDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkAttachmentDescriptionStencilLoadOp, VkAttachmentDescriptionDifferentLoadStoreOpShouldDifferHashAndCompare)
{
    VkAttachmentDescription desc = GetDefaultVkAttachmentDescription();

    VkAttachmentDescription desc2 = GetDefaultVkAttachmentDescription();
    desc2.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_LOAD;

    EXPECT_NE(VkAttachmentDescriptionHashFn{}(desc), VkAttachmentDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkAttachmentDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkAttachmentDescriptionStencilStoreOp, VkAttachmentDescriptionDifferentStencilStoreOpShouldDifferHashAndCompare)
{
    VkAttachmentDescription desc = GetDefaultVkAttachmentDescription();

    VkAttachmentDescription desc2 = GetDefaultVkAttachmentDescription();
    desc2.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_STORE;

    EXPECT_NE(VkAttachmentDescriptionHashFn{}(desc), VkAttachmentDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkAttachmentDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkAttachmentDescriptionInitialLayout, VkAttachmentDescriptionDifferentInitialLayoutShouldDifferHashAndCompare)
{
    VkAttachmentDescription desc = GetDefaultVkAttachmentDescription();

    VkAttachmentDescription desc2 = GetDefaultVkAttachmentDescription();
    desc2.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;

    EXPECT_NE(VkAttachmentDescriptionHashFn{}(desc), VkAttachmentDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkAttachmentDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkAttachmentDescriptionFinalLayout, VkAttachmentDescriptionDifferentFinalLayoutShouldDifferHashAndCompare)
{
    VkAttachmentDescription desc = GetDefaultVkAttachmentDescription();

    VkAttachmentDescription desc2 = GetDefaultVkAttachmentDescription();
    desc2.finalLayout             = VK_IMAGE_LAYOUT_UNDEFINED;

    EXPECT_NE(VkAttachmentDescriptionHashFn{}(desc), VkAttachmentDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkAttachmentDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkAttachmentDescriptionCollisionTest, VkAttachmentDescriptionDifferentFinalLayoutShouldDifferHashAndCompare)
{
    VkAttachmentDescription desc = GetDefaultVkAttachmentDescription();
    desc.format                  = VK_FORMAT_R4G4_UNORM_PACK8; // 1
    desc.samples                 = VK_SAMPLE_COUNT_1_BIT; // 1

    VkAttachmentDescription desc2 = GetDefaultVkAttachmentDescription();
    desc2.format                  = VK_FORMAT_UNDEFINED; // 0
    desc2.samples                 = VK_SAMPLE_COUNT_2_BIT; // 2

    EXPECT_NE(VkAttachmentDescriptionHashFn{}(desc), VkAttachmentDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkAttachmentDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkAttachmentReference, VkAttachmentReferenceShouldHaveSameHashAndCompare)
{
    VkAttachmentReference ref{ 0, VK_IMAGE_LAYOUT_UNDEFINED };
    VkAttachmentReference ref2{ 0, VK_IMAGE_LAYOUT_UNDEFINED };

    EXPECT_EQ(VkAttachmentReferenceHashFn{}(ref), VkAttachmentReferenceHashFn{}(ref2));
    EXPECT_TRUE(VkAttachmentReferenceEqualFn{}(ref, ref2));
}

TEST(UnitVkAttachmentReferenceIndex, VkAttachmentReferenceIndexShouldDifferHashAndCompare)
{
    VkAttachmentReference ref{ 0, VK_IMAGE_LAYOUT_UNDEFINED };
    VkAttachmentReference ref2{ 1, VK_IMAGE_LAYOUT_UNDEFINED };

    EXPECT_NE(VkAttachmentReferenceHashFn{}(ref), VkAttachmentReferenceHashFn{}(ref2));
    EXPECT_FALSE(VkAttachmentReferenceEqualFn{}(ref, ref2));
}

TEST(UnitVkAttachmentReferenceLayout, VkAttachmentReferenceLayoutShouldDifferHashAndCompare)
{
    VkAttachmentReference ref{ 0, VK_IMAGE_LAYOUT_UNDEFINED };
    VkAttachmentReference ref2{ 0, VK_IMAGE_LAYOUT_GENERAL };

    EXPECT_NE(VkAttachmentReferenceHashFn{}(ref), VkAttachmentReferenceHashFn{}(ref2));
    EXPECT_FALSE(VkAttachmentReferenceEqualFn{}(ref, ref2));
}

TEST(UnitVkAttachmentReferenceLayoutCollisionTest, VkAttachmentReferenceLayoutShouldDifferHashAndCompare)
{
    VkAttachmentReference ref{ 1, VK_IMAGE_LAYOUT_UNDEFINED };
    VkAttachmentReference ref2{ 0, VK_IMAGE_LAYOUT_GENERAL };

    EXPECT_NE(VkAttachmentReferenceHashFn{}(ref), VkAttachmentReferenceHashFn{}(ref2));
    EXPECT_FALSE(VkAttachmentReferenceEqualFn{}(ref, ref2));
}

TEST(UnitVkSubpassDependency, VkSubpassDependencyShouldHaveSameHashAndCompare) { VkSubpassDependency dep; }

TEST(UnitVkSubpassDescription, VkSubpassDescriptionShouldHaveSameHashAndCompare)
{
    std::vector<VkAttachmentReference> colorAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    std::vector<VkAttachmentReference> depthStencilAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    VkSubpassDescription               desc = GetDefaultVkSubpassDescription(colorAttachments, depthStencilAttachments);

    std::vector<VkAttachmentReference> colorAttachments2{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    std::vector<VkAttachmentReference> depthStencilAttachments2{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    VkSubpassDescription               desc2 = GetDefaultVkSubpassDescription(colorAttachments2, depthStencilAttachments2);

    EXPECT_EQ(VkSubpassDescriptionHashFn{}(desc), VkSubpassDescriptionHashFn{}(desc2));
    EXPECT_TRUE(VkSubpassDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkSubpassDescriptionFlags, VkSubpassDescriptionFlagsShouldDifferHashAndCompare)
{
    std::vector<VkAttachmentReference> colorAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    std::vector<VkAttachmentReference> depthStencilAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    VkSubpassDescription               desc = GetDefaultVkSubpassDescription(colorAttachments, depthStencilAttachments);

    std::vector<VkAttachmentReference> colorAttachments2{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    std::vector<VkAttachmentReference> depthStencilAttachments2{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    VkSubpassDescription               desc2 = GetDefaultVkSubpassDescription(colorAttachments2, depthStencilAttachments2);
    desc2.flags                              = 1;

    EXPECT_NE(VkSubpassDescriptionHashFn{}(desc), VkSubpassDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkSubpassDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkSubpassDescriptionPipelineBindPoint, VkSubpassDescriptionPipelineBindPointShouldDifferHashAndCompare)
{
    std::vector<VkAttachmentReference> colorAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    std::vector<VkAttachmentReference> depthStencilAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    VkSubpassDescription               desc = GetDefaultVkSubpassDescription(colorAttachments, depthStencilAttachments);

    std::vector<VkAttachmentReference> colorAttachments2{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    std::vector<VkAttachmentReference> depthStencilAttachments2{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    VkSubpassDescription               desc2 = GetDefaultVkSubpassDescription(colorAttachments2, depthStencilAttachments2);
    desc2.pipelineBindPoint                  = VK_PIPELINE_BIND_POINT_COMPUTE;

    EXPECT_NE(VkSubpassDescriptionHashFn{}(desc), VkSubpassDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkSubpassDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkSubpassDescriptionInputAttachmentCount, VkSubpassDescriptionInputAttachmentCountShouldDifferHashAndCompare)
{
    std::vector<VkAttachmentReference> colorAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    std::vector<VkAttachmentReference> depthStencilAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    VkSubpassDescription               desc = GetDefaultVkSubpassDescription(colorAttachments, depthStencilAttachments);

    std::vector<VkAttachmentReference> colorAttachments2{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    std::vector<VkAttachmentReference> depthStencilAttachments2{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    VkSubpassDescription               desc2 = GetDefaultVkSubpassDescription(colorAttachments2, depthStencilAttachments2);
    desc2.inputAttachmentCount               = 1;
    desc2.pInputAttachments                  = colorAttachments2.data();

    EXPECT_NE(VkSubpassDescriptionHashFn{}(desc), VkSubpassDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkSubpassDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkSubpassDescriptionInputAttachment, VkSubpassDescriptionInputAttachmentShouldDifferHashAndCompare)
{
    std::vector<VkAttachmentReference> colorAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    std::vector<VkAttachmentReference> depthStencilAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    VkSubpassDescription               desc = GetDefaultVkSubpassDescription(colorAttachments, depthStencilAttachments);
    desc.inputAttachmentCount               = 1;
    desc.pInputAttachments                  = colorAttachments.data();

    std::vector<VkAttachmentReference> colorAttachments2{ { 1, VK_IMAGE_LAYOUT_UNDEFINED } };
    std::vector<VkAttachmentReference> depthStencilAttachments2{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    VkSubpassDescription               desc2 = GetDefaultVkSubpassDescription(colorAttachments2, depthStencilAttachments2);
    desc2.inputAttachmentCount               = 1;
    desc2.pInputAttachments                  = colorAttachments2.data();

    EXPECT_NE(VkSubpassDescriptionHashFn{}(desc), VkSubpassDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkSubpassDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkSubpassDescriptionInputAttachment2, VkSubpassDescriptionInputAttachmentShouldDifferHashAndCompare)
{
    std::vector<VkAttachmentReference> colorAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    std::vector<VkAttachmentReference> depthStencilAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    VkSubpassDescription               desc = GetDefaultVkSubpassDescription(colorAttachments, depthStencilAttachments);
    desc.inputAttachmentCount               = 1;
    desc.pInputAttachments                  = colorAttachments.data();

    std::vector<VkAttachmentReference> colorAttachments2{ { 0, VK_IMAGE_LAYOUT_GENERAL } };
    std::vector<VkAttachmentReference> depthStencilAttachments2{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    VkSubpassDescription               desc2 = GetDefaultVkSubpassDescription(colorAttachments2, depthStencilAttachments2);
    desc2.inputAttachmentCount               = 1;
    desc2.pInputAttachments                  = colorAttachments2.data();

    EXPECT_NE(VkSubpassDescriptionHashFn{}(desc), VkSubpassDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkSubpassDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkSubpassDescriptionColorAttachmentCount, VkSubpassDescriptionColorAttachmentCountShouldDifferHashAndCompare)
{
    std::vector<VkAttachmentReference> colorAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    std::vector<VkAttachmentReference> depthStencilAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    VkSubpassDescription               desc = GetDefaultVkSubpassDescription(colorAttachments, depthStencilAttachments);

    std::vector<VkAttachmentReference> colorAttachments2{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    std::vector<VkAttachmentReference> depthStencilAttachments2{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    VkSubpassDescription               desc2 = GetDefaultVkSubpassDescription(colorAttachments2, depthStencilAttachments2);
    desc2.colorAttachmentCount               = 0;

    EXPECT_NE(VkSubpassDescriptionHashFn{}(desc), VkSubpassDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkSubpassDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkSubpassDescriptionColorAttachment, VkSubpassDescriptionColorAttachmentShouldDifferHashAndCompare)
{
    std::vector<VkAttachmentReference> colorAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    std::vector<VkAttachmentReference> depthStencilAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    VkSubpassDescription               desc = GetDefaultVkSubpassDescription(colorAttachments, depthStencilAttachments);

    std::vector<VkAttachmentReference> colorAttachments2{ { 1, VK_IMAGE_LAYOUT_UNDEFINED } };
    std::vector<VkAttachmentReference> depthStencilAttachments2{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    VkSubpassDescription               desc2 = GetDefaultVkSubpassDescription(colorAttachments2, depthStencilAttachments2);

    EXPECT_NE(VkSubpassDescriptionHashFn{}(desc), VkSubpassDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkSubpassDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkSubpassDescriptionColorAttachment2, VkSubpassDescriptionColorAttachmentShouldDifferHashAndCompare)
{
    std::vector<VkAttachmentReference> colorAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    std::vector<VkAttachmentReference> depthStencilAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    VkSubpassDescription               desc = GetDefaultVkSubpassDescription(colorAttachments, depthStencilAttachments);

    std::vector<VkAttachmentReference> colorAttachments2{ { 0, VK_IMAGE_LAYOUT_GENERAL } };
    std::vector<VkAttachmentReference> depthStencilAttachments2{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    VkSubpassDescription               desc2 = GetDefaultVkSubpassDescription(colorAttachments2, depthStencilAttachments2);

    EXPECT_NE(VkSubpassDescriptionHashFn{}(desc), VkSubpassDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkSubpassDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkSubpassDescriptionResolveAttachments, VkSubpassDescriptionResolveAttachmentsShouldDifferHashAndCompare)
{
    std::vector<VkAttachmentReference> colorAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    std::vector<VkAttachmentReference> depthStencilAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    VkSubpassDescription               desc = GetDefaultVkSubpassDescription(colorAttachments, depthStencilAttachments);

    std::vector<VkAttachmentReference> colorAttachments2{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    std::vector<VkAttachmentReference> depthStencilAttachments2{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    VkSubpassDescription               desc2 = GetDefaultVkSubpassDescription(colorAttachments2, depthStencilAttachments2);
    desc2.pResolveAttachments                = colorAttachments2.data();

    EXPECT_NE(VkSubpassDescriptionHashFn{}(desc), VkSubpassDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkSubpassDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkSubpassDescriptionDepthStencilAttachment, VkSubpassDescriptionDepthStencilAttachmentShouldDifferHashAndCompare)
{
    std::vector<VkAttachmentReference> colorAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    std::vector<VkAttachmentReference> depthStencilAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    VkSubpassDescription               desc = GetDefaultVkSubpassDescription(colorAttachments, depthStencilAttachments);

    std::vector<VkAttachmentReference> colorAttachments2{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    std::vector<VkAttachmentReference> depthStencilAttachments2{ { 1, VK_IMAGE_LAYOUT_UNDEFINED } };
    VkSubpassDescription               desc2 = GetDefaultVkSubpassDescription(colorAttachments2, depthStencilAttachments2);

    EXPECT_NE(VkSubpassDescriptionHashFn{}(desc), VkSubpassDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkSubpassDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkSubpassDescriptionDepthStencilAttachment2, VkSubpassDescriptionDepthStencilAttachmentShouldDifferHashAndCompare)
{
    std::vector<VkAttachmentReference> colorAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    std::vector<VkAttachmentReference> depthStencilAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    VkSubpassDescription               desc = GetDefaultVkSubpassDescription(colorAttachments, depthStencilAttachments);

    std::vector<VkAttachmentReference> colorAttachments2{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    std::vector<VkAttachmentReference> depthStencilAttachments2{ { 0, VK_IMAGE_LAYOUT_GENERAL } };
    VkSubpassDescription               desc2 = GetDefaultVkSubpassDescription(colorAttachments2, depthStencilAttachments2);

    EXPECT_NE(VkSubpassDescriptionHashFn{}(desc), VkSubpassDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkSubpassDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkSubpassDescriptionPreserveAttachmentCount, VkSubpassDescriptionPreserveAttachmentCountShouldDifferHashAndCompare)
{
    std::vector<VkAttachmentReference> colorAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    std::vector<VkAttachmentReference> depthStencilAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    VkSubpassDescription               desc = GetDefaultVkSubpassDescription(colorAttachments, depthStencilAttachments);

    std::vector<VkAttachmentReference> colorAttachments2{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    std::vector<VkAttachmentReference> depthStencilAttachments2{ { 0, VK_IMAGE_LAYOUT_GENERAL } };
    VkSubpassDescription               desc2 = GetDefaultVkSubpassDescription(colorAttachments2, depthStencilAttachments2);
    desc2.preserveAttachmentCount            = 1;
    const uint32_t preserve                  = 1;
    desc2.pPreserveAttachments               = &preserve;

    EXPECT_NE(VkSubpassDescriptionHashFn{}(desc), VkSubpassDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkSubpassDescriptionEqualFn{}(desc, desc2));
}

TEST(UnitVkSubpassDescriptionPreserveAttachment, VkSubpassDescriptionPreserveAttachmentShouldDifferHashAndCompare)
{
    std::vector<VkAttachmentReference> colorAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    std::vector<VkAttachmentReference> depthStencilAttachments{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    VkSubpassDescription               desc     = GetDefaultVkSubpassDescription(colorAttachments, depthStencilAttachments);
    const uint32_t                     preserve = 0;
    desc.pPreserveAttachments                   = &preserve;

    std::vector<VkAttachmentReference> colorAttachments2{ { 0, VK_IMAGE_LAYOUT_UNDEFINED } };
    std::vector<VkAttachmentReference> depthStencilAttachments2{ { 0, VK_IMAGE_LAYOUT_GENERAL } };
    VkSubpassDescription               desc2 = GetDefaultVkSubpassDescription(colorAttachments2, depthStencilAttachments2);
    desc2.preserveAttachmentCount            = 1;
    const uint32_t preserve2                 = 1;
    desc2.pPreserveAttachments               = &preserve2;

    EXPECT_NE(VkSubpassDescriptionHashFn{}(desc), VkSubpassDescriptionHashFn{}(desc2));
    EXPECT_FALSE(VkSubpassDescriptionEqualFn{}(desc, desc2));
}

///---------------------------------------------------------------------------------------------------------------------------------------
// The following tests are limited because the correct tests are above

TEST(UnitRenderPassCaching, EmptyRenderPassInfoShouldHaveSameHashAndBeEqual)
{
    RIVkRenderPassInfo       info;
    const RIVkRenderPassInfo expected;

    const auto infoHash     = RIRenderPassInfoHashFn{}(info);
    const auto expectedHash = RIRenderPassInfoHashFn{}(expected);
    EXPECT_EQ(infoHash, expectedHash);
    EXPECT_TRUE(RIRenderPassInfoEqualFn{}(info, expected));
}

TEST(UnitRenderPassCaching, SomeRenderPassInfoShouldHaveSameHashAndBeEqual)
{
    RIVkRenderPassInfo info;
    RIVkRenderPassInfo expected;
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
