// Copyright RedFox Studio 2022

#include "VulkanDevice7.h"

#include "asserts.h"

#include "UtilsVK.h"

namespace Fox
{

RIVulkanDevice7::~RIVulkanDevice7() { check(_framebuffers.size() == 0); }

VkFramebuffer
RIVulkanDevice7::CreateFramebuffer(const std::vector<VkImageView>& imageViews, uint32_t width, uint32_t height, VkRenderPass renderpass)
{
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass      = renderpass;
    framebufferInfo.attachmentCount = (uint32_t)imageViews.size();
    framebufferInfo.pAttachments    = imageViews.data();
    framebufferInfo.width           = width;
    framebufferInfo.height          = height;
    framebufferInfo.layers          = 1; /// default

    VkFramebuffer  framebuffer{};
    const VkResult result = vkCreateFramebuffer(Device, &framebufferInfo, nullptr, &framebuffer);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkUtils::VkErrorString(result));
        }

    _framebuffers.insert(framebuffer);

    return framebuffer;
}

void
RIVulkanDevice7::DestroyFramebuffer(VkFramebuffer framebuffer)
{
    vkDestroyFramebuffer(Device, framebuffer, nullptr);
    _framebuffers.erase(_framebuffers.find(framebuffer));
}

}