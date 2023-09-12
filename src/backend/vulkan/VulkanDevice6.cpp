// Copyright RedFox Studio 2022

#include "VulkanDevice6.h"

#include "asserts.h"

#include "UtilsVK.h"

namespace Fox
{

RIVulkanDevice6::~RIVulkanDevice6() { check(_samplers.size() == 0); }

VkSampler
RIVulkanDevice6::CreateSampler(VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode mode, float minLod, float maxLod, VkSamplerMipmapMode mipmapMode, bool anisotropy, float maxAnisotropy)
{
    // Create the sampler
    VkSampler sampler{};
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter               = minFilter;
        samplerInfo.minFilter               = magFilter;
        samplerInfo.addressModeU            = mode;
        samplerInfo.addressModeV            = mode;
        samplerInfo.addressModeW            = mode;
        samplerInfo.anisotropyEnable        = anisotropy ? VK_TRUE : VK_FALSE;
        samplerInfo.maxAnisotropy           = maxAnisotropy;
        samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable           = VK_FALSE;
        samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias              = 0.f;
        samplerInfo.minLod                  = minLod;
        samplerInfo.maxLod                  = maxLod;

        const VkResult result = vkCreateSampler(Device, &samplerInfo, nullptr, &sampler);
        if (VKFAILED(result))
            {
                throw std::runtime_error(VkUtils::VkErrorString(result));
                return nullptr;
            }
    }

    _samplers.insert(sampler);

    return sampler;
}

void
RIVulkanDevice6::DestroySampler(VkSampler sampler)
{
    vkDestroySampler(Device, sampler, nullptr);
    _samplers.erase(_samplers.find(sampler));
}

}