// Copyright RedFox Studio 2022

#pragma once

#include "RICacheMap.h"
#include "RIResource.h"
#include "VulkanDevice12.h"

#include <volk.h>

#include <list>

namespace Fox
{
struct RIVkRenderPassInfo
{
    std::vector<VkAttachmentDescription> AttachmentDescription;
    std::vector<VkAttachmentReference>   ColorAttachmentReference;
    std::vector<VkAttachmentReference>   DepthStencilAttachmentReference;
    std::vector<VkSubpassDescription>    SubpassDescription;
    std::vector<VkSubpassDependency>     SubpassDependency;
};

struct VkAttachmentDescriptionHashFn
{
    size_t operator()(const VkAttachmentDescription& attachmentDesc) const
    {
        std::size_t seed = 0;

        // Hash individual members of VkAttachmentDescription
        std::hash<VkFormat>              formatHash;
        std::hash<VkSampleCountFlagBits> samplesHash;
        std::hash<VkAttachmentLoadOp>    loadOpHash;
        std::hash<VkAttachmentStoreOp>   storeOpHash;
        std::hash<VkAttachmentLoadOp>    stencilLoadOpHash;
        std::hash<VkAttachmentStoreOp>   stencilStoreOpHash;
        std::hash<VkImageLayout>         initialLayoutHash;
        std::hash<VkImageLayout>         finalLayoutHash;

        seed ^= formatHash(attachmentDesc.format);
        seed ^= samplesHash(attachmentDesc.samples);
        seed ^= loadOpHash(attachmentDesc.loadOp);
        seed ^= storeOpHash(attachmentDesc.storeOp);
        seed ^= stencilLoadOpHash(attachmentDesc.stencilLoadOp);
        seed ^= stencilStoreOpHash(attachmentDesc.stencilStoreOp);
        seed ^= initialLayoutHash(attachmentDesc.initialLayout);
        seed ^= finalLayoutHash(attachmentDesc.finalLayout);

        return seed;
    };
};

struct VkAttachmentDescriptionEqualFn
{
    bool operator()(const VkAttachmentDescription& lhs, const VkAttachmentDescription& rhs) const
    {
        return (lhs.flags == rhs.flags && lhs.format == rhs.format && lhs.samples == rhs.samples && lhs.loadOp == rhs.loadOp && lhs.storeOp == rhs.storeOp && lhs.stencilLoadOp == rhs.stencilLoadOp &&
        lhs.stencilStoreOp == rhs.stencilStoreOp && lhs.initialLayout == rhs.initialLayout && lhs.finalLayout == rhs.finalLayout);
    }
};

struct VkAttachmentReferenceHashFn
{
    size_t operator()(const VkAttachmentReference& lhs) const
    {
        std::hash<uint32_t> uint32Hasher;
        const auto          hash1 = uint32Hasher(lhs.attachment) ^ 0x9e3779b9;
        const auto          hash2 = uint32Hasher(static_cast<uint32_t>(lhs.layout)) ^ 0x9e3779b1;
        return (hash1 + hash2) ^ (hash2 - hash1);
    };
};

struct VkAttachmentReferenceEqualFn
{
    bool operator()(const VkAttachmentReference& lhs, const VkAttachmentReference& rhs) const { return (lhs.attachment == rhs.attachment && lhs.layout == rhs.layout); }
};

struct VkSubpassDescriptionHashFn
{
    size_t operator()(const VkSubpassDescription& subpassDesc) const
    {
        std::size_t seed = 0;

        seed ^= std::hash<uint32_t>{}(subpassDesc.flags) ^ 0x9e3779b9;
        seed += std::hash<VkPipelineBindPoint>{}(subpassDesc.pipelineBindPoint) ^ 0x9e3779b1;
        seed ^= std::hash<uint32_t>{}(subpassDesc.inputAttachmentCount);
        for (uint32_t i = 0; i < subpassDesc.inputAttachmentCount; i++)
            {
                const VkAttachmentReference* ref = subpassDesc.pInputAttachments + i;
                seed ^= VkAttachmentReferenceHashFn{}(*ref);
            }
        seed += std::hash<uint32_t>{}(subpassDesc.colorAttachmentCount) ^ 0x9e3779b9;
        for (uint32_t i = 0; i < subpassDesc.colorAttachmentCount; i++)
            {
                const VkAttachmentReference* ref = subpassDesc.pColorAttachments + i;
                seed ^= VkAttachmentReferenceHashFn{}(*ref);
            }
        if (subpassDesc.pResolveAttachments != nullptr)
            {
                const auto hash = VkAttachmentReferenceHashFn{}(*subpassDesc.pResolveAttachments);
                seed += hash;
            }

        if (subpassDesc.pDepthStencilAttachment != nullptr)
            {
                const auto hash = VkAttachmentReferenceHashFn{}(*subpassDesc.pDepthStencilAttachment);
                seed += hash;
            }

        seed += std::hash<uint32_t>{}(subpassDesc.preserveAttachmentCount) ^ 0x9e3779b9;
        for (uint32_t i = 0; i < subpassDesc.preserveAttachmentCount; i++)
            {
                const uint32_t* ref = subpassDesc.pPreserveAttachments + i;
                seed ^= std::hash<uint32_t>{}(*ref);
            }

        return seed;
    }
};

struct VkSubpassDescriptionEqualFn
{
    bool operator()(const VkSubpassDescription& lhs, const VkSubpassDescription& rhs) const
    {
        // Compare each member of the VkSubpassDescription
        if (lhs.flags != rhs.flags || lhs.pipelineBindPoint != rhs.pipelineBindPoint || lhs.inputAttachmentCount != rhs.inputAttachmentCount || lhs.colorAttachmentCount != rhs.colorAttachmentCount ||
        lhs.preserveAttachmentCount != rhs.preserveAttachmentCount)
            {
                return false;
            }
        // if they are different
        if (lhs.pResolveAttachments != rhs.pResolveAttachments)
            { // but one of them is nullptr
                if (lhs.pResolveAttachments == nullptr || rhs.pResolveAttachments == nullptr)
                    {
                        return false;
                    }
            }

        // Compare input attachments
        for (uint32_t i = 0; i < lhs.inputAttachmentCount; ++i)
            {
                if (!VkAttachmentReferenceEqualFn{}(lhs.pInputAttachments[i], rhs.pInputAttachments[i]))
                    {
                        return false;
                    }
            }

        // Compare color attachments
        for (uint32_t i = 0; i < lhs.colorAttachmentCount; ++i)
            {
                if (!VkAttachmentReferenceEqualFn{}(lhs.pColorAttachments[i], rhs.pColorAttachments[i]))
                    {
                        return false;
                    }
                if (lhs.pResolveAttachments != nullptr && rhs.pResolveAttachments != nullptr)
                    {
                        if (!VkAttachmentReferenceEqualFn{}(lhs.pResolveAttachments[i], rhs.pResolveAttachments[i]))
                            {
                                return false;
                            }
                    }
            }

        // Compare depth/stencil attachment
        if (lhs.pDepthStencilAttachment != nullptr && rhs.pDepthStencilAttachment != nullptr)
            {
                if (!VkAttachmentReferenceEqualFn{}(*lhs.pDepthStencilAttachment, *rhs.pDepthStencilAttachment))
                    {
                        return false;
                    }
            }

        for (uint32_t i = 0; i < lhs.preserveAttachmentCount; ++i)
            {
                if (lhs.pPreserveAttachments[i] != rhs.pPreserveAttachments[i])
                    {
                        return false;
                    }
            }
        return true;
    }
};

struct VkSubpassDependencyHashFn
{
    size_t operator()(const VkSubpassDependency& dependency) const
    {
        // Combine hashes of individual members
        std::size_t seed = 0;

        seed ^= std::hash<uint32_t>{}(dependency.srcSubpass);
        seed ^= std::hash<uint32_t>{}(dependency.dstSubpass);
        seed ^= std::hash<VkPipelineStageFlags>{}(dependency.srcStageMask);
        seed ^= std::hash<VkPipelineStageFlags>{}(dependency.dstStageMask);
        seed ^= std::hash<VkAccessFlags>{}(dependency.srcAccessMask);
        seed ^= std::hash<VkAccessFlags>{}(dependency.dstAccessMask);
        seed ^= std::hash<VkDependencyFlags>{}(dependency.dependencyFlags);

        return seed;
    }
};

struct VkSubpassDependencyEqualFn
{
    bool operator()(const VkSubpassDependency& lhs, const VkSubpassDependency& rhs) const
    {
        return (lhs.srcSubpass == rhs.srcSubpass && lhs.dstSubpass == rhs.dstSubpass && lhs.srcStageMask == rhs.srcStageMask && lhs.dstStageMask == rhs.dstStageMask &&
        lhs.srcAccessMask == rhs.srcAccessMask && lhs.dstAccessMask == rhs.dstAccessMask && lhs.dependencyFlags == rhs.dependencyFlags);
    }
};

struct RIRenderPassInfoHashFn
{
    size_t operator()(const RIVkRenderPassInfo& lhs) const
    {
        size_t hash{};

        for (const VkAttachmentDescription& desc : lhs.AttachmentDescription)
            {
                hash ^= VkAttachmentDescriptionHashFn{}(desc);
            }
        // Already internally hashes the attachment references
        for (const VkSubpassDescription& desc : lhs.SubpassDescription)
            {
                hash ^= VkSubpassDescriptionHashFn{}(desc);
            }
        for (const VkSubpassDependency& desc : lhs.SubpassDependency)
            {
                hash ^= VkSubpassDependencyHashFn{}(desc);
            }
        return hash;
    }
};

struct RIRenderPassInfoEqualFn
{
    bool operator()(const RIVkRenderPassInfo& lhs, const RIVkRenderPassInfo& rhs) const
    {
        if (lhs.AttachmentDescription.size() != rhs.AttachmentDescription.size() || lhs.ColorAttachmentReference.size() != rhs.ColorAttachmentReference.size() ||
        lhs.DepthStencilAttachmentReference.size() != rhs.DepthStencilAttachmentReference.size() || lhs.SubpassDescription.size() != rhs.SubpassDescription.size() ||
        lhs.SubpassDependency.size() != rhs.SubpassDependency.size())
            {
                return false;
            }

        for (size_t i = 0; i < lhs.AttachmentDescription.size(); i++)
            {
                if (!VkAttachmentDescriptionEqualFn{}(lhs.AttachmentDescription[i], rhs.AttachmentDescription[i]))
                    {
                        return false;
                    }
            }
        // Already compares inside the colorAttachment depthStencilAttachment and preserveAttachment
        for (size_t i = 0; i < lhs.SubpassDescription.size(); i++)
            {
                if (!VkSubpassDescriptionEqualFn{}(lhs.SubpassDescription[i], rhs.SubpassDescription[i]))
                    {
                        return false;
                    }
            }

        for (size_t i = 0; i < lhs.SubpassDependency.size(); i++)
            {
                if (!VkSubpassDependencyEqualFn{}(lhs.SubpassDependency[i], rhs.SubpassDependency[i]))
                    {
                        return false;
                    }
            }

        return true;
    }
};

class RIVulkanDevice13 : public RIVulkanDevice12
{
  public:
    virtual ~RIVulkanDevice13();

    VkResult CreateRenderPass(const RIVkRenderPassInfo& info, VkRenderPass* renderPass);
    void     DestroyRenderPass(VkRenderPass renderPass);

  private:
    RICacheMap<RIVkRenderPassInfo, VkRenderPass, RIRenderPassInfoHashFn, RIRenderPassInfoEqualFn> _renderPassMap;
};
}