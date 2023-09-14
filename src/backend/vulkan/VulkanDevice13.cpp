// Copyright RedFox Studio 2022

#include "VulkanDevice13.h"

#include "asserts.h"

#include "UtilsVK.h"

namespace Fox
{

RIVulkanDevice13::~RIVulkanDevice13(){ check(_renderPassMap.Size() == 0) }

VkResult RIVulkanDevice13::CreateRenderPass(const RIVkRenderPassInfo& info, VkRenderPass* renderPass)
{
    auto cached = _renderPassMap.Find(info);
    if (cached)
        {
            *renderPass = cached;
            return VK_SUCCESS;
        }

    VkRenderPassCreateInfo createInfo{};
    createInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.pNext           = nullptr;
    createInfo.flags           = 0;
    createInfo.attachmentCount = (uint32_t)info.AttachmentDescription.size();
    createInfo.pAttachments    = info.AttachmentDescription.data();
    createInfo.subpassCount    = (uint32_t)info.SubpassDescription.size();
    createInfo.pSubpasses      = info.SubpassDescription.data();
    createInfo.dependencyCount = (uint32_t)info.SubpassDependency.size();
    createInfo.pDependencies   = info.SubpassDependency.data();

    const VkResult result = vkCreateRenderPass(Device, &createInfo, nullptr, renderPass);
    _renderPassMap.Add(info, *renderPass);

    return result;
}

void
RIVulkanDevice13::DestroyRenderPass(VkRenderPass renderPass)
{
    vkDestroyRenderPass(Device, renderPass, nullptr);
    _renderPassMap.EraseByValue(renderPass);
}

}