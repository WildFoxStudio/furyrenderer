// Copyright RedFox Studio 2022

#pragma once

#include "VulkanDevice6.h"

#include <volk.h>

#include <functional>
#include <unordered_set>

namespace Fox
{

// images implementation
class RIVulkanDevice7 : public RIVulkanDevice6
{

  public:
    virtual ~RIVulkanDevice7();
    VkFramebuffer _createFramebuffer(const std::vector<VkImageView>& imageViews, uint32_t width, uint32_t height, VkRenderPass renderpass);
    void          _destroyFramebuffer(VkFramebuffer framebuffer);

  private:
    std::unordered_set<VkFramebuffer> _framebuffers_DEPRECATED;
};
}