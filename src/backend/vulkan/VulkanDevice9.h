// Copyright RedFox Studio 2022

#pragma once

#include "VulkanDevice8.h"
#include "RICacheMap.h"

#include <volk.h>

namespace Fox
{

struct RIPipelineLayoutInfo
{
    std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;
    std::vector<VkPushConstantRange>   PushConstantRanges;

    inline std::size_t hash_combine(std::size_t seed, std::size_t value) const { return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2)); }
    size_t             hash() const
    {
        size_t hash = 0;
        for (const auto ptr : DescriptorSetLayouts)
            {
                std::size_t seed = 0;
                seed ^= std::hash<VkDescriptorSetLayout>{}(ptr) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                hash = hash_combine(seed, hash);
            }
        for (const auto p : PushConstantRanges)
            {
                std::size_t seed = 0;
                seed ^= std::hash<uint32_t>{}(p.size) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                seed ^= std::hash<uint32_t>{}(p.offset) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                seed ^= std::hash<VkShaderStageFlags>{}(p.stageFlags) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                hash = hash_combine(seed, hash);
            }
        return hash;
    };
};

struct RIPipelineLayoutHasher
{
    // Custom hash function for multiple size_t values

    size_t operator()(const RIPipelineLayoutInfo& info) const { return info.hash(); };
};

class RIPipelineLayoutEqualFn
{
  public:
    bool operator()(const RIPipelineLayoutInfo& t1, const RIPipelineLayoutInfo& t2) const
    {
        if (t1.DescriptorSetLayouts.size() != t2.DescriptorSetLayouts.size())
            return false;
        if (t1.PushConstantRanges.size() != t2.PushConstantRanges.size())
            return false;

        for (size_t i = 0; i < t1.DescriptorSetLayouts.size(); i++)
            {
                if (!(t1.DescriptorSetLayouts[i] == t2.DescriptorSetLayouts[i]))
                    return false;
            }
        for (size_t i = 0; i < t1.PushConstantRanges.size(); i++)
            {
                if (!(t1.PushConstantRanges[i].offset == t2.PushConstantRanges[i].offset))
                    return false;
                if (!(t1.PushConstantRanges[i].size == t2.PushConstantRanges[i].size))
                    return false;
                if (!(t1.PushConstantRanges[i].stageFlags == t2.PushConstantRanges[i].stageFlags))
                    return false;
            }

        return true;
    }
};

// images implementation
class RIVulkanDevice9 : public RIVulkanDevice8
{

  public:
    virtual ~RIVulkanDevice9();

    VkPipelineLayout CreatePipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayout, const std::vector<VkPushConstantRange>& pushConstantRange);
    void             DestroyPipelineLayout(VkPipelineLayout pipelineLayout);

  private:
    RICacheMap<RIPipelineLayoutInfo, VkPipelineLayout, RIPipelineLayoutHasher, RIPipelineLayoutEqualFn> _pipelineLayoutCache;
};
}