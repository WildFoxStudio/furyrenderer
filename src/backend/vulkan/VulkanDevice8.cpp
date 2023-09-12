// Copyright RedFox Studio 2022

#include "VulkanDevice8.h"

#include "asserts.h"

#include "UtilsVK.h"

namespace Fox
{

RIVulkanDevice8::~RIVulkanDevice8() { check(_descriptorSetLayoutCache.size() == 0); }

VkDescriptorSetLayoutBinding
RIVulkanDevice8::CreateDescriptorSetLayoutBindingUniformBufferDynamic(uint32_t binding, uint32_t descriptorCount, VkShaderStageFlags stages)
{
    VkDescriptorSetLayoutBinding b{};
    b.binding            = binding;
    b.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    b.descriptorCount    = descriptorCount;
    b.stageFlags         = stages;
    b.pImmutableSamplers = nullptr;

    return b;
}

VkDescriptorSetLayoutBinding
RIVulkanDevice8::CreateDescriptorSetLayoutBindingCombinedImageSampler(uint32_t binding, uint32_t descriptorCount, VkShaderStageFlags stages)
{
    VkDescriptorSetLayoutBinding b{};
    b.binding            = binding;
    b.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    b.descriptorCount    = descriptorCount;
    b.stageFlags         = stages;
    b.pImmutableSamplers = nullptr;

    return b;
}

VkDescriptorSetLayout
RIVulkanDevice8::CreateDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
    // Find inside cache
    const auto cached = _descriptorSetLayoutCache.find(bindings);
    if (cached != _descriptorSetLayoutCache.end())
        {
            return cached->second;
        }

    // Create new one
    VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = (uint32_t)bindings.size();
    layoutCreateInfo.pBindings    = bindings.data();

    VkDescriptorSetLayout layout{};
    const VkResult        result = vkCreateDescriptorSetLayout(Device, &layoutCreateInfo, nullptr, &layout);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkUtils::VkErrorString(result));
        }

    // cache it
    _descriptorSetLayoutCache[bindings] = layout;

    return layout;
}

void
RIVulkanDevice8::DestroyDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout)
{
    vkDestroyDescriptorSetLayout(Device, descriptorSetLayout, nullptr);

    // explicit remove from cache
    for (const auto& pair : _descriptorSetLayoutCache)
        {
            if (pair.second == descriptorSetLayout)
                {
                    _descriptorSetLayoutCache.erase(pair.first);
                    break;
                }
        }
}

}