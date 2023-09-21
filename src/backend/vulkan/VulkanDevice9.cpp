// Copyright RedFox Studio 2022

#include "VulkanDevice9.h"

#include "asserts.h"

#include "UtilsVK.h"

namespace Fox
{

RIVulkanDevice9::~RIVulkanDevice9() { check(_pipelineLayoutCache.Size() == 0); }

VkPipelineLayout
RIVulkanDevice9::CreatePipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayout, const std::vector<VkPushConstantRange>& pushConstantRange)
{
    VkPipelineLayout cached = _pipelineLayoutCache.Find(RIPipelineLayoutInfo{ descriptorSetLayout, pushConstantRange });
    if (cached != nullptr)
        {
            return cached;
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
            throw std::runtime_error(VkUtils::VkErrorString(result));
        }

    RIPipelineLayoutInfo pipelineInfo{ descriptorSetLayout, pushConstantRange };
    _pipelineLayoutCache.Add(std::move(pipelineInfo), pipelineLayout);

    return pipelineLayout;
}

void
RIVulkanDevice9::DestroyPipelineLayout(VkPipelineLayout pipelineLayout)
{
    vkDestroyPipelineLayout(Device, pipelineLayout, nullptr);

    _pipelineLayoutCache.EraseByValue(pipelineLayout);
}

}