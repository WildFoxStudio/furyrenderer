// Copyright RedFox Studio 2022

#pragma once

#include "VulkanDevice9.h"

#include <volk.h>

#include <unordered_set>

namespace Fox
{

struct RIVulkanPipelineBuilder
{
    RIVulkanPipelineBuilder(const std::vector<VkPipelineShaderStageCreateInfo>& shaders,
    const std::vector<VkVertexInputBindingDescription>&                         vertexInputBinding,
    const std::vector<VkVertexInputAttributeDescription>&                       vertexInputAttributes,
    VkPipelineLayout                                                            pipelineLayout,
    VkRenderPass                                                                renderPass);

    void AddViewport(VkViewport viewport);
    void AddScissor(VkRect2D);
    void SetTopology(VkPrimitiveTopology topology);
    void SetAlphaBlending();
    void SetDynamicState(const std::vector<VkDynamicState>& states);
    void SetDepthTesting(bool testDepth, bool writeDepth);
    void SetDepthTestingOp(VkCompareOp compareOp = VK_COMPARE_OP_LESS);
    void SetDepthBounds(float min = 0.f, float max = 1.f);
    void SetSuperSampling(VkSampleCountFlagBits count = VK_SAMPLE_COUNT_1_BIT);
    void SetPolygonMode(VkPolygonMode mode = VK_POLYGON_MODE_FILL);
    void SetCulling(VkCullModeFlags mode);
    void SetDepthStencil(bool enabled);
    void SetDepthStencilOp(VkStencilOpState op);

    std::vector<VkPipelineShaderStageCreateInfo>     PipelineShaderStages;
    VkPipelineVertexInputStateCreateInfo             PipelineVertexInput{};
    std::vector<VkVertexInputBindingDescription>     InputBindings;
    std::vector<VkVertexInputAttributeDescription>   VertexInputAttributes;
    VkPipelineInputAssemblyStateCreateInfo           PipelineInputAssembly{};
    std::vector<VkViewport>                          Viewports;
    std::vector<VkRect2D>                            Scissors;
    VkPipelineViewportStateCreateInfo                PipelineViewportState{};
    VkPipelineRasterizationStateCreateInfo           PipelineRasterization{};
    VkPipelineMultisampleStateCreateInfo             PipelineMultisampleState{};
    VkPipelineDepthStencilStateCreateInfo            PipelineDepthStencilState{};
    std::vector<VkPipelineColorBlendAttachmentState> PipelineColorBlendAttachmentStates;
    bool                                             UsingDefaultBlendState{ true };
    VkPipelineColorBlendStateCreateInfo              PipelineColorBlendeState{};
    std::vector<VkDynamicState>                      DynamicStates;
    VkPipelineDynamicStateCreateInfo                 PipelineDynamicState{};
    VkGraphicsPipelineCreateInfo                     PipelineCreateInfo{};

    void _fillColorBlendState();
    void _fillDynamicState();
    void _fillViewportState();
};

// images implementation
class RIVulkanDevice10 : public RIVulkanDevice9
{

  public:
    virtual ~RIVulkanDevice10();
    VkPipeline CreatePipeline(const VkGraphicsPipelineCreateInfo* info);
    void       DestroyPipeline(VkPipeline pipeline);

  private:
    std::unordered_set<VkPipeline> _pipelines;
};
}