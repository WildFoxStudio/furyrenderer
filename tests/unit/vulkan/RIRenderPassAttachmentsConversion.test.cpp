#include "IContext.h"
#include "backend/vulkan/UtilsVK.h"
#include "backend/vulkan/VulkanContext.h"

#include <gtest/gtest.h>

using namespace Fox;

VkSubpassDependency
GetExpectedVkSubpassDependency()
{
    VkSubpassDependency dependency{};
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    return dependency;
}

TEST(UnitRenderPassConversionBasic, BasicRenderPassShoudConvertAsExpected)
{
    DRenderPassAttachments attachments;
    {
        DRenderPassAttachment att(EFormat::R8G8B8A8_UNORM,
        ESampleBit::COUNT_1_BIT,
        ERenderPassLoad::Clear,
        ERenderPassStore::Store,
        ERenderPassLayout::Undefined,
        ERenderPassLayout::Present,
        EAttachmentReference::COLOR_ATTACHMENT);

        attachments.Attachments.emplace_back(att);
    }

    const auto result = ConvertRenderPassAttachmentsToRIVkRenderPassInfo(attachments);

    {
        ASSERT_EQ(result.AttachmentDescription.size(), 1);

        VkAttachmentDescription expectedDescription;
        expectedDescription.flags   = 0;
        expectedDescription.format  = VK_FORMAT_R8G8B8A8_UNORM;
        expectedDescription.samples = VK_SAMPLE_COUNT_1_BIT;
        expectedDescription.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        expectedDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        expectedDescription.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // Stencil not used right now
        expectedDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Stencil not used right now

        expectedDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        expectedDescription.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        EXPECT_TRUE(VkAttachmentDescriptionEqualFn{}(expectedDescription, result.AttachmentDescription.front()));
    }

    {
        ASSERT_EQ(result.ColorAttachmentReference.size(), 1);

        VkAttachmentReference expectedRef;
        expectedRef.attachment = 0;
        expectedRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        EXPECT_TRUE(VkAttachmentReferenceEqualFn{}(expectedRef, result.ColorAttachmentReference.front()));
    }
    {
        ASSERT_EQ(result.DepthStencilAttachmentReference.size(), 0);
    }

    {
        ASSERT_EQ(result.SubpassDependency.size(), 1);

        VkSubpassDependency expectedDependency = GetExpectedVkSubpassDependency();

        EXPECT_TRUE(VkSubpassDependencyEqualFn{}(expectedDependency, result.SubpassDependency.front()));
    }

    {
        ASSERT_EQ(result.SubpassDescription.size(), 1);

        VkSubpassDescription subpass{};
        subpass.flags                   = 0;
        subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.inputAttachmentCount    = 0;
        subpass.pInputAttachments       = nullptr;
        subpass.colorAttachmentCount    = 1;
        subpass.pColorAttachments       = result.ColorAttachmentReference.data();
        subpass.pResolveAttachments     = NULL;
        subpass.pDepthStencilAttachment = nullptr;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments    = NULL;

        EXPECT_TRUE(VkSubpassDescriptionEqualFn{}(subpass, result.SubpassDescription.front()));
    }
}

TEST(UnitRenderPassConversionBasic2, BasicRenderPassShoudConvertAsExpected)
{
    DRenderPassAttachments attachments;
    {
        DRenderPassAttachment att(EFormat::R8G8B8A8_UNORM,
        ESampleBit::COUNT_1_BIT,
        ERenderPassLoad::Clear,
        ERenderPassStore::Store,
        ERenderPassLayout::Undefined,
        ERenderPassLayout::Present,
        EAttachmentReference::COLOR_ATTACHMENT);

        attachments.Attachments.emplace_back(att);
    }
    {
        DRenderPassAttachment att(EFormat::DEPTH32_FLOAT_STENCIL8_UINT,
        ESampleBit::COUNT_1_BIT,
        ERenderPassLoad::Clear,
        ERenderPassStore::Store,
        ERenderPassLayout::Undefined,
        ERenderPassLayout::AsAttachment,
        EAttachmentReference::DEPTH_STENCIL_ATTACHMENT);

        attachments.Attachments.emplace_back(att);
    }

    const auto result = ConvertRenderPassAttachmentsToRIVkRenderPassInfo(attachments);

    ASSERT_EQ(result.AttachmentDescription.size(), 2);

    {
        VkAttachmentDescription expectedDescription;
        expectedDescription.flags   = 0;
        expectedDescription.format  = VK_FORMAT_R8G8B8A8_UNORM;
        expectedDescription.samples = VK_SAMPLE_COUNT_1_BIT;
        expectedDescription.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        expectedDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        expectedDescription.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // Stencil not used right now
        expectedDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Stencil not used right now

        expectedDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        expectedDescription.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        EXPECT_TRUE(VkAttachmentDescriptionEqualFn{}(expectedDescription, result.AttachmentDescription.front()));
    }
    {

        VkAttachmentDescription expectedDescription;
        expectedDescription.flags   = 0;
        expectedDescription.format  = VK_FORMAT_D32_SFLOAT_S8_UINT;
        expectedDescription.samples = VK_SAMPLE_COUNT_1_BIT;
        expectedDescription.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        expectedDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        expectedDescription.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // Stencil not used right now
        expectedDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Stencil not used right now

        expectedDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        expectedDescription.finalLayout   = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;

        EXPECT_TRUE(VkAttachmentDescriptionEqualFn{}(expectedDescription, result.AttachmentDescription.back()));
    }

    {
        ASSERT_EQ(result.ColorAttachmentReference.size(), 1);

        VkAttachmentReference expectedRef;
        expectedRef.attachment = 0;
        expectedRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        EXPECT_TRUE(VkAttachmentReferenceEqualFn{}(expectedRef, result.ColorAttachmentReference.front()));
    }
    {
        ASSERT_EQ(result.DepthStencilAttachmentReference.size(), 1);
        VkAttachmentReference expectedRef;
        expectedRef.attachment = 1;
        expectedRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        EXPECT_TRUE(VkAttachmentReferenceEqualFn{}(expectedRef, result.DepthStencilAttachmentReference.front()));
    }

    {
        ASSERT_EQ(result.SubpassDependency.size(), 1);

        VkSubpassDependency expectedDependency = GetExpectedVkSubpassDependency();

        EXPECT_TRUE(VkSubpassDependencyEqualFn{}(expectedDependency, result.SubpassDependency.front()));
    }

    {
        ASSERT_EQ(result.SubpassDescription.size(), 1);

        VkSubpassDescription subpass{};
        subpass.flags                   = 0;
        subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.inputAttachmentCount    = 0;
        subpass.pInputAttachments       = nullptr;
        subpass.colorAttachmentCount    = 1;
        subpass.pColorAttachments       = result.ColorAttachmentReference.data();
        subpass.pResolveAttachments     = NULL;
        subpass.pDepthStencilAttachment = result.DepthStencilAttachmentReference.data();
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments    = NULL;

        EXPECT_TRUE(VkSubpassDescriptionEqualFn{}(subpass, result.SubpassDescription.front()));
    }
}