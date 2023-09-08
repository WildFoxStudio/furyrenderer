// Copyright RedFox Studio 2022

#include "VulkanDevice10.h"

#include "Core/utils/debug.h"
#include "UtilsVK.h"

namespace Fox
{

RIVulkanPipelineBuilder::RIVulkanPipelineBuilder(const std::vector<VkPipelineShaderStageCreateInfo>& shaders,
const std::vector<VkVertexInputBindingDescription>&                                                  vertexInputBinding,
const std::vector<VkVertexInputAttributeDescription>&                                                vertexInputAttributes,
VkPipelineLayout                                                                                     pipelineLayout,
VkRenderPass                                                                                         renderPass)
  : DynamicStates{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR }
{

    PipelineShaderStages  = shaders;
    InputBindings         = vertexInputBinding;
    VertexInputAttributes = vertexInputAttributes;

    {
        PipelineVertexInput.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        PipelineVertexInput.pNext                           = nullptr;
        PipelineVertexInput.flags                           = NULL;
        PipelineVertexInput.vertexBindingDescriptionCount   = (uint32_t)InputBindings.size();
        PipelineVertexInput.pVertexBindingDescriptions      = InputBindings.data();
        PipelineVertexInput.vertexAttributeDescriptionCount = (uint32_t)VertexInputAttributes.size();
        PipelineVertexInput.pVertexAttributeDescriptions    = VertexInputAttributes.data();
    }
    {
        PipelineInputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        PipelineInputAssembly.pNext                  = nullptr;
        PipelineInputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        PipelineInputAssembly.primitiveRestartEnable = VK_FALSE;
    }
    {
        _fillViewportState();
    }
    {
        // Rasterizer info
        PipelineRasterization.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        PipelineRasterization.depthClampEnable        = VK_FALSE;
        PipelineRasterization.rasterizerDiscardEnable = VK_FALSE;
        PipelineRasterization.polygonMode             = VK_POLYGON_MODE_FILL;
        // PipelineRasterization.polygonMode             = VK_POLYGON_MODE_LINE;
        PipelineRasterization.lineWidth               = 1.0f;
        PipelineRasterization.cullMode                = VK_CULL_MODE_NONE;
        PipelineRasterization.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        PipelineRasterization.depthBiasEnable         = VK_FALSE;
        PipelineRasterization.depthBiasConstantFactor = 0.0f; // Optional
        PipelineRasterization.depthBiasClamp          = 0.0f; // Optional
        PipelineRasterization.depthBiasSlopeFactor    = 0.0f; // Optional
    }
    {
        PipelineMultisampleState.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        PipelineMultisampleState.sampleShadingEnable   = VK_FALSE;
        PipelineMultisampleState.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
        PipelineMultisampleState.minSampleShading      = 1.0f; // Optional
        PipelineMultisampleState.pSampleMask           = nullptr; // Optional
        PipelineMultisampleState.alphaToCoverageEnable = VK_FALSE; // Optional
        PipelineMultisampleState.alphaToOneEnable      = VK_FALSE; // Optional
    }
    {
        // Depth stencil
        PipelineDepthStencilState.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        PipelineDepthStencilState.depthTestEnable       = VK_TRUE;
        PipelineDepthStencilState.depthWriteEnable      = VK_TRUE;
        PipelineDepthStencilState.depthCompareOp        = VK_COMPARE_OP_LESS;
        PipelineDepthStencilState.depthBoundsTestEnable = VK_FALSE;
        PipelineDepthStencilState.stencilTestEnable     = VK_FALSE;
        PipelineDepthStencilState.minDepthBounds        = 0.f;
        PipelineDepthStencilState.maxDepthBounds        = 1.f;
    }
    {
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable         = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD; // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD; // Optional
        PipelineColorBlendAttachmentStates.push_back(colorBlendAttachment);
    }
    {
        _fillColorBlendState();
    }
    {
        _fillDynamicState();
    }

    PipelineCreateInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    PipelineCreateInfo.stageCount          = (uint32_t)PipelineShaderStages.size();
    PipelineCreateInfo.pStages             = PipelineShaderStages.data();
    PipelineCreateInfo.pVertexInputState   = &PipelineVertexInput;
    PipelineCreateInfo.pInputAssemblyState = &PipelineInputAssembly;
    PipelineCreateInfo.pViewportState      = &PipelineViewportState;
    PipelineCreateInfo.pRasterizationState = &PipelineRasterization;
    PipelineCreateInfo.pMultisampleState   = &PipelineMultisampleState;
    PipelineCreateInfo.pDepthStencilState  = &PipelineDepthStencilState;
    PipelineCreateInfo.pColorBlendState    = &PipelineColorBlendeState;
    PipelineCreateInfo.pDynamicState       = &PipelineDynamicState; // Optional
    PipelineCreateInfo.layout              = pipelineLayout;
    PipelineCreateInfo.renderPass          = renderPass;
    PipelineCreateInfo.subpass             = 0;
    PipelineCreateInfo.basePipelineHandle  = VK_NULL_HANDLE; // Optional
    PipelineCreateInfo.basePipelineIndex   = -1; // Optional
}

void
RIVulkanPipelineBuilder::AddViewport(VkViewport viewport)
{
    Viewports.push_back(viewport);
    _fillViewportState();
}

void
RIVulkanPipelineBuilder::AddScissor(VkRect2D scissor)
{
    Scissors.push_back(scissor);
    _fillViewportState();
}

void
RIVulkanPipelineBuilder::SetTopology(VkPrimitiveTopology topology)
{
    PipelineInputAssembly.topology = topology;
}

void
RIVulkanPipelineBuilder::SetAlphaBlending()
{
    if (UsingDefaultBlendState)
        {
            PipelineColorBlendAttachmentStates.clear();
        }

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable         = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

    PipelineColorBlendAttachmentStates.push_back(colorBlendAttachment);

    _fillColorBlendState();
}

void
RIVulkanPipelineBuilder::SetDynamicState(const std::vector<VkDynamicState>& states)
{
    DynamicStates = states;
    _fillDynamicState();
}

void
RIVulkanPipelineBuilder::SetDepthTesting(bool testDepth, bool writeDepth)
{
    PipelineDepthStencilState.depthTestEnable  = testDepth ? VK_TRUE : VK_FALSE;
    PipelineDepthStencilState.depthWriteEnable = writeDepth ? VK_TRUE : VK_FALSE;
}

void
RIVulkanPipelineBuilder::SetDepthTestingOp(VkCompareOp compareOp)
{
    PipelineDepthStencilState.depthCompareOp = compareOp;
}

void
RIVulkanPipelineBuilder::SetDepthBounds(float min, float max)
{
    PipelineDepthStencilState.minDepthBounds = min;
    PipelineDepthStencilState.maxDepthBounds = max;
}

void
RIVulkanPipelineBuilder::SetSuperSampling(VkSampleCountFlagBits count)
{
    PipelineMultisampleState.rasterizationSamples = count;
}

void
RIVulkanPipelineBuilder::SetPolygonMode(VkPolygonMode mode)
{
    PipelineRasterization.polygonMode = mode;
}

void
RIVulkanPipelineBuilder::SetCulling(VkCullModeFlags mode)
{
    PipelineRasterization.cullMode = mode;
}

void
RIVulkanPipelineBuilder::SetDepthStencil(bool enabled)
{
    PipelineDepthStencilState.stencilTestEnable = enabled ? VK_TRUE : VK_FALSE;
}

void
RIVulkanPipelineBuilder::SetDepthStencilOp(VkStencilOpState op)
{
    PipelineDepthStencilState.front = op;
}

void
RIVulkanPipelineBuilder::_fillColorBlendState()
{
    PipelineColorBlendeState.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    PipelineColorBlendeState.logicOpEnable     = VK_FALSE;
    PipelineColorBlendeState.logicOp           = VK_LOGIC_OP_COPY; // Optional
    PipelineColorBlendeState.attachmentCount   = (uint32_t)PipelineColorBlendAttachmentStates.size();
    PipelineColorBlendeState.pAttachments      = PipelineColorBlendAttachmentStates.data();
    PipelineColorBlendeState.blendConstants[0] = 0.0f; // Optional
    PipelineColorBlendeState.blendConstants[1] = 0.0f; // Optional
    PipelineColorBlendeState.blendConstants[2] = 0.0f; // Optional
    PipelineColorBlendeState.blendConstants[3] = 0.0f; // Optional
}

void
RIVulkanPipelineBuilder::_fillDynamicState()
{
    PipelineDynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    PipelineDynamicState.pNext             = NULL;
    PipelineDynamicState.flags             = NULL;
    PipelineDynamicState.dynamicStateCount = (uint32_t)DynamicStates.size();
    PipelineDynamicState.pDynamicStates    = DynamicStates.data();
}

void
RIVulkanPipelineBuilder::_fillViewportState()
{
    PipelineViewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    PipelineViewportState.viewportCount = (uint32_t)Viewports.size();
    PipelineViewportState.pViewports    = Viewports.data();
    PipelineViewportState.scissorCount  = (uint32_t)Scissors.size();
    PipelineViewportState.pScissors     = Scissors.data();
}

RIVulkanDevice10::~RIVulkanDevice10(){ check(_pipelines.size() == 0) }

VkPipeline RIVulkanDevice10::CreatePipeline(const VkGraphicsPipelineCreateInfo* info)
{
    VkPipeline     pipeline{};
    const VkResult result = vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, info, nullptr, &pipeline);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkErrorString(result));
        }

    _pipelines.insert(pipeline);

    return pipeline;
}

void
RIVulkanDevice10::DestroyPipeline(VkPipeline pipeline)
{
    check(pipeline);
    check(_pipelines.size() > 0);
    vkDestroyPipeline(Device, pipeline, nullptr);

    _pipelines.erase(_pipelines.find(pipeline));
}

//
// VkVertexInputAttributeDescription
// RIVulkanDevice10::CreateVertexAttribute(uint32_t index, uint32_t binding, VkFormat format, uint32_t offset)
//{
//    VkVertexInputAttributeDescription attr{};
//    attr.location = index; // uint32_t
//    attr.binding  = binding; // uint32_t
//    attr.format   = format; // VkFormat
//    attr.offset   = offset; // uint32_t
//
//    return attr;
//}
//
// VkVertexInputBindingDescription
// RIVulkanDevice10::CreateInputBinding(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate)
//{
//    VkVertexInputBindingDescription inputBinding{};
//    inputBinding.binding   = binding;
//    inputBinding.stride    = stride;
//    inputBinding.inputRate = inputRate;
//    return inputBinding;
//}
//
// VkPipelineVertexInputStateCreateInfo
// RIVulkanDevice10::CreateVertexInput(const std::vector<VkVertexInputBindingDescription>& inputBindings, const std::vector<VkVertexInputAttributeDescription>& attributes)
//{
//    VkPipelineVertexInputStateCreateInfo info{};
//    info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//    info.pNext                           = nullptr;
//    info.flags                           = NULL;
//    info.vertexBindingDescriptionCount   = (uint32_t)inputBindings.size();
//    info.pVertexBindingDescriptions      = inputBindings.data();
//    info.vertexAttributeDescriptionCount = (uint32_t)attributes.size();
//    info.pVertexAttributeDescriptions    = attributes.data();
//
//    return info;
//}
//
// VkPipelineInputAssemblyStateCreateInfo
// RIVulkanDevice10::CreateInputAssembly(VkPrimitiveTopology topology)
//{
//    // Input assembly
//    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
//    inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
//    inputAssembly.topology               = topology;
//    inputAssembly.primitiveRestartEnable = VK_FALSE; // idk
//    return inputAssembly;
//}
//
// VkViewport
// RIVulkanDevice10::CreateViewport(uint32_t width, uint32_t height, float znear, float zfar)
//{
//    check(zfar < 1.1f); // range 0 - 1
//
//    VkViewport v{};
//    v.width    = width;
//    v.height   = height;
//    v.minDepth = znear;
//    v.maxDepth = zfar;
//
//    return v;
//}
//
// VkRect2D
// RIVulkanDevice10::CreateScissorNoScissor(uint32_t width, uint32_t height)
//{
//    return VkRect2D{ { 0, 0 }, { width, height } };
//}

}