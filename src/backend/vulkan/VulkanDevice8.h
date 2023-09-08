// Copyright RedFox Studio 2022

#pragma once

#include "VulkanDevice7.h"

#include <volk.h>

#include <unordered_map>

namespace Fox
{

struct RIDescriptorSetLayoutBindingsHasher
{
    // Custom hash function for multiple size_t values
    inline std::size_t hash_combine(std::size_t seed, std::size_t value) const { return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2)); }
    size_t             operator()(const std::vector<VkDescriptorSetLayoutBinding>& bindings) const
    {
        size_t hash = 0;
        for (const auto& binding : bindings)
            {
                std::size_t seed = 0;
                seed ^= std::hash<uint32_t>{}(binding.binding) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                seed ^= std::hash<VkDescriptorType>{}(binding.descriptorType) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                seed ^= std::hash<uint32_t>{}(binding.descriptorCount) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                seed ^= std::hash<VkShaderStageFlags>{}(binding.stageFlags) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                hash = hash_combine(seed, hash);
            }
        return hash;
    };
};

class RIDescriptorSetLayoutBindingsEqualFn
{
  public:
    bool operator()(const std::vector<VkDescriptorSetLayoutBinding>& t1, const std::vector<VkDescriptorSetLayoutBinding>& t2) const
    {
        if (t1.size() != t2.size())
            return false;

        for (size_t i = 0; i < t1.size(); i++)
            {
                if (!(t1[i].binding == t2[i].binding))
                    return false;
                if (!(t1[i].descriptorCount == t2[i].descriptorCount))
                    return false;
                if (!(t1[i].descriptorType == t2[i].descriptorType))
                    return false;
                if (!(t1[i].stageFlags == t2[i].stageFlags))
                    return false;
                if (!(t1[i].pImmutableSamplers == t2[i].pImmutableSamplers))
                    return false;
            }

        return true;
    }
};

// images implementation
class RIVulkanDevice8 : public RIVulkanDevice7
{

  public:
    virtual ~RIVulkanDevice8();

    VkDescriptorSetLayoutBinding CreateDescriptorSetLayoutBindingUniformBufferDynamic(uint32_t binding, uint32_t descriptorCount, VkShaderStageFlags stages);
    VkDescriptorSetLayoutBinding CreateDescriptorSetLayoutBindingCombinedImageSampler(uint32_t binding, uint32_t descriptorCount, VkShaderStageFlags stages);
    VkDescriptorSetLayout        CreateDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings);
    void                         DestroyDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout);

  private:
    std::unordered_map<std::vector<VkDescriptorSetLayoutBinding>, VkDescriptorSetLayout, RIDescriptorSetLayoutBindingsHasher, RIDescriptorSetLayoutBindingsEqualFn> _descriptorSetLayoutCache;
};
}