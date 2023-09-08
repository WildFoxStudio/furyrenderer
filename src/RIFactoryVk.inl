// Copyright RedFox Studio 2022

#pragma once

#if defined(FOX_USE_VULKAN)

#include <cstring>
#include <vector>

#include "Core/utils/Filter.h"
#include "Core/utils/Fox_Assert.h"
#include "RenderInterface/vulkan/RIFactoryVk.h"
#include "RenderInterface/vulkan/UtilsVK.h"

namespace Fox
{

VkPipelineShaderStageCreateInfo
RIShaderStageBuilderVk::Create(VkShaderStageFlagBits stage, VkShaderModule shaderModule)
{
    check(shaderModule);
    check(stage == VK_SHADER_STAGE_VERTEX_BIT || stage == VK_SHADER_STAGE_FRAGMENT_BIT);

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.pNext               = nullptr;
    stageInfo.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.flags               = NULL;
    stageInfo.stage               = stage;
    stageInfo.module              = shaderModule;
    stageInfo.pName               = PENTRYPOINTNAME;
    stageInfo.pSpecializationInfo = nullptr;

    return stageInfo;
};

VkResult
RIShaderModuleBuilderVk::Create(VkDevice device, const std::vector<unsigned char>& byteCode, VkShaderModule* shaderModule)
{
    check(device);
    check(byteCode.size() > 0);
    check(shaderModule && !*shaderModule);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = byteCode.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(byteCode.data());

    return vkCreateShaderModule(device, &createInfo, nullptr, shaderModule);
};
inline std::vector<VkVertexInputAttributeDescription>
RIVertexInputAttributeDescriptionBuilderVk::Create(const std::vector<RIInputSlot>& inputs)
{
    std::vector<VkVertexInputAttributeDescription> vertexInputAttribute;
    vertexInputAttribute.reserve(inputs.size());

    unsigned int location = 0;
    for (const auto& input : inputs)
        {
            VkVertexInputAttributeDescription attr{};
            attr.location = location++; // uint32_t
            attr.binding  = 0; // uint32_t
            attr.format   = EFormat_TO_VkFormat[(int)input.Format]; // VkFormat
            attr.offset   = input.ByteOffset; // uint32_t

            // Emplace
            vertexInputAttribute.emplace_back(std::move(attr));
        }
    return vertexInputAttribute;
}

}
#endif