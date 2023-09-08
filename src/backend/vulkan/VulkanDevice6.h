// Copyright RedFox Studio 2022

#pragma once

#include "VulkanDevice5.h"

#include <volk.h>

#include <functional>
#include <unordered_set>

namespace Fox
{

// images implementation
class RIVulkanDevice6 : public RIVulkanDevice5
{

  public:
    virtual ~RIVulkanDevice6();

    VkSampler CreateSampler(VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode mode, float minLod, float maxLod, VkSamplerMipmapMode mipmapMode, bool anisotropy, float maxAnisotropy);
    void      DestroySampler(VkSampler sampler);
  private:
    std::unordered_set<VkSampler> _samplers;
};
}