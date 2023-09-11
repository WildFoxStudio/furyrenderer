// Copyright RedFox Studio 2022

#include "VulkanDevice9.h"

#include "asserts.h"

#include "UtilsVK.h"

namespace Fox
{

RIVulkanDevice9::~RIVulkanDevice9() { check(_pipelineLayoutCache.size() == 0); }

VkPipelineLayout
RIVulkanDevice9::CreatePipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayout, const std::vector<VkPushConstantRange>& pushConstantRange)
{
    const auto& cache = _pipelineLayoutCache.find({ descriptorSetLayout, pushConstantRange });
    if (cache != _pipelineLayoutCache.end())
        {
            return cache->second;
        }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount         = (uint32_t)descriptorSetLayout.size();
    pipelineLayoutInfo.pSetLayouts            = descriptorSetLayout.data();
    pipelineLayoutInfo.pushConstantRangeCount = (uint32_t)pushConstantRange.size();
    pipelineLayoutInfo.pPushConstantRanges    = pushConstantRange.data();

    VkPipelineLayout pipelineLayout{};
    const VkResult   result = vkCreatePipelineLayout(Device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkErrorString(result));
        }

    _pipelineLayoutCache[RIPipelineLayoutInfo{ descriptorSetLayout, pushConstantRange }] = pipelineLayout;

    return pipelineLayout;
}

void
RIVulkanDevice9::DestroyPipelineLayout(VkPipelineLayout pipelineLayout)
{
    vkDestroyPipelineLayout(Device, pipelineLayout, nullptr);

    for (const auto& pair : _pipelineLayoutCache)
        {
            if (pair.second == pipelineLayout)
                {
                    _pipelineLayoutCache.erase(pair.first);
                    break;
                }
        }
}

}