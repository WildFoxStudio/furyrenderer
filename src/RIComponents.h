// Copyright RedFox Studio 2022

#pragma once

#if defined(FOX_USE_VULKAN)

#include "Core/renderInterface/RenderInterface.h"
#include "Core/window/WindowBase.h"

#ifdef FOX_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(FOX_LINUX)
#define VK_USE_PLATFORM_XLIB_KHR
#endif

#define VK_NO_PROTOTYPES
// Volk usage https://gpuopen.com/learn/reducing-vulkan-api-call-overhead/
#include <volk.h>
#include <vulkan/vulkan.h>

namespace Fox
{

inline static constexpr VkDescriptorType RIShaderBlockParam_TO_VkDescriptorType[4] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER };

inline static constexpr VkFormat EImageFormat_TO_VkFormat[5] = { VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_D32_SFLOAT_S8_UINT };

inline static constexpr VkFormat EFormat_TO_VkFormat[3] = { VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R32G32_SFLOAT };

struct RIObject : public RIComponent
{
    virtual ~RIObject() = default;
};

struct RIBuffer
{
    std::vector<VkBuffer>       Buffers;
    std::vector<VkDeviceMemory> Memory;
};

struct RIVertexBufferVk : RIVertexBuffer
{
    RIBuffer Buffer;
};

struct RIFenceVk : public RIFence
{
    RIFenceVk(bool signaleted);
    void                      WaitForSignal(uint64_t timeoutNanoseconds) override;
    void                      Reset() override;
    VkFence                   Fence{};
    VkDevice Device_DEPRECATED{};
};

struct RISemaphoreVk : public RISemaphore
{
    VkSemaphore Semaphore{};
};

struct RIRenderPassVk : public RIRenderPass
{
    VkRenderPass RenderPass{};
};

struct RIFramebufferVk : public RIFramebuffer
{
    VkFramebuffer Framebuffer{};
    uint32_t      Width, Height;
    bool          HasDepthStencil{};
};

struct RITextureVk : public RITexture
{
    explicit RITextureVk(EImageFormat format, EType type, const size_t size) : RITexture(format, type, size){};
    virtual ~RITextureVk() = default;

    VkImage        Image{};
    VkDeviceMemory DeviceMemory{};
    VkImageView    ImageView{};
    VkSampler      Sampler{};
};

struct RIPipelineVk : public RIPipeline
{
    VkPipeline       Pipeline{};
    VkPipelineLayout PipelineLayout{};
    inline bool      IsValid() const { return (Pipeline && PipelineLayout); };
};

struct RIShaderDescriptorSetVk : public RIShaderDescriptorsSet
{
    VkDescriptorSet DescriptorSet;
};

struct RIShaderDescriptorsSetPoolVk : public RIShaderDescriptorsSetPool
{
    RIShaderDescriptorsSetPoolVk(const std::unordered_map<RIShaderDescriptorBindings::Type, uint32_t>& typeToCountMap) : RIShaderDescriptorsSetPool(typeToCountMap){};

    VkDescriptorPool DescriptorPool{};
    uint32_t         CurrentlyAllocated{ 0 };
};

struct RIShaderDescriptorsSetLayoutVk : public RIShaderDescriptorsSetLayout
{
    VkDescriptorSetLayout Layout{};
};

struct RISwapchainVk : public RISwapchain
{
    inline static const std::vector<VkPresentModeKHR> PresentationModes = {
        VkPresentModeKHR::VK_PRESENT_MODE_IMMEDIATE_KHR, /*Immediate*/
        VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR, /*Hybrid vsync*/
        VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR, /*vsync*/
        VkPresentModeKHR::VK_PRESENT_MODE_FIFO_RELAXED_KHR, /*like FIFO except it behaves like IMMEDIATE in the case that it had no image in queue for the last VBLANK swap (i.e. prefers tearing rather
                                                               than showing old image).*/
    };

    inline static const std::vector<VkSurfaceFormatKHR> Formats = { VkSurfaceFormatKHR{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
        VkSurfaceFormatKHR{ VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
        VkSurfaceFormatKHR{ VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR } };

    inline static const std::vector<VkFormat> DepthStencilFormats = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };

    AWindow*                    WindowPtr{};
    uint32_t                    FramesInFlight{ 0 };
    VkSurfaceFormatKHR          Format{ VkFormat::VK_FORMAT_MAX_ENUM, VkColorSpaceKHR::VK_COLOR_SPACE_MAX_ENUM_KHR };
    VkPresentModeKHR            PresentMode{ VkPresentModeKHR::VK_PRESENT_MODE_MAX_ENUM_KHR };
    VkExtent2D                  Extent{ 0, 0 };
    VkSurfaceKHR                Surface{};
    VkSwapchainKHR              Swapchain{};
    VkFormat                    DepthStencilFormat{}; // TODO: To remove since it's duplicated
    std::vector<RIFramebuffer*> Framebuffers;
    std::vector<RITextureVk*>   ImageTextures;
    RITextureVk*                DepthStencilTexture;
    std::vector<RISemaphore*>   ImageAvailableSemaphores;
    DLinearColor                ClearColor;

    inline std::vector<RISemaphore*>   GetImageAvailableSemaphore() const override { return ImageAvailableSemaphores; };
    inline std::vector<RIFramebuffer*> GetFramebuffers() const override { return Framebuffers; };
    inline RITexture*                  GetImage(int frameIndex) const override { return ImageTextures[frameIndex]; };
    inline EImageFormat                GetImageFormat() const override
    {
        if (Format.format == VK_FORMAT_B8G8R8A8_UNORM)
            return EImageFormat::BGRA8_UNORM;
        if (Format.format == VK_FORMAT_B8G8R8A8_SRGB)
            return EImageFormat::BGRA8_SRGB;

        critical(0);
    };

    void GetExtents(uint32_t& width, uint32_t& height) const override
    {
        width  = Extent.width;
        height = Extent.height;
    };

    inline bool IsValid() const { return (WindowPtr != nullptr); };
};

// Requirements : https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap15.html#interfaces-resources-layout
struct RIUniformBufferObjectVk : public RIUniformBufferObject
{
    RIUniformBufferObjectVk(size_t dataSize) : RIUniformBufferObject(dataSize){};

    RIBuffer Buffer;
};

struct RIRenderCommandListVk : public RIRenderCommandList
{
    VkCommandBuffer CommandBuffer{};
    void            Reset() override;

    void Begin(RIRenderPass* renderPass = nullptr, RIFramebuffer* framebuffer = nullptr) override;
    void End() override;

    void BeginRenderPass(RIRenderPass* renderpass, RIFramebuffer* framebuffer, bool recordSecondaryBuffer = false, const DLinearColor& clearColor = DLinearColor(0, 0, 0)) override;
    void EndRenderPass() override;

    void BindGraphicsPipeline(RIPipeline* pipeline) override;
    void BindVertexBuffer(RIVertexBuffer* buffer) override;

    void Draw(size_t vertexCount, size_t instanceCount, size_t firstVertex, size_t firstInstance) override;

    void SetViewport(const DViewport& viewport) override;
    void SetScissor(const DViewport& viewport) override;

    void PushConstants(RIPipeline* pipeline, size_t size, void* data) override;

    void BindDescriptorSets(uint32_t firstSet, std::vector<RIShaderDescriptorsSet*> descriptorSets, RIPipeline* pipeline) override;

    void ExecuteSecondaryCommandList(std::vector<RIRenderCommandList*> commandList) override;

    // missing
    // ClearRenderTargetView(_swapChainRenderTarget.get(), dcolor);
    // ClearDepthStencilView(_swapChainDepthStencilTarget.get());
  private:
    bool _recordingState{ false };
    bool isRenderPassBound{ false };

    inline bool IsReadyForDrawingCommand() const
    {
        if (IsPrimary)
            return (_recordingState) && (isRenderPassBound);
        else

            return _recordingState;
    };
    inline bool IsRecording() const { return (_recordingState); };
    inline bool IsDefaultState() const { return (!_recordingState); };
    inline bool IsRenderPassBound() const { return IsPrimary ? isRenderPassBound : true; };
};

inline void
RIRenderCommandListVk::Reset()
{
    check(IsDefaultState());

    VkCommandBufferResetFlags flags{};
    vkResetCommandBuffer(CommandBuffer, flags);
    WasRecorded = false;
}

inline void
RIRenderCommandListVk::Begin(RIRenderPass* renderPass, RIFramebuffer* framebuffer)
{
    check(renderPass && framebuffer || !renderPass && !framebuffer);
    // Must not be in recording state
    check(IsDefaultState());
    _recordingState = true;

    // Begin command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = IsPrimary ? NULL : VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT; // Optional

    VkCommandBufferInheritanceInfo inheritanceInfo{};
    inheritanceInfo.sType                = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritanceInfo.pNext                = nullptr;
    inheritanceInfo.renderPass           = renderPass ? static_cast<RIRenderPassVk*>(renderPass)->RenderPass : nullptr;
    inheritanceInfo.subpass              = 0;
    inheritanceInfo.framebuffer          = framebuffer ? static_cast<RIFramebufferVk*>(framebuffer)->Framebuffer : nullptr;
    inheritanceInfo.occlusionQueryEnable = VK_FALSE;
    inheritanceInfo.queryFlags           = 0;
    inheritanceInfo.pipelineStatistics   = 0;

    beginInfo.pInheritanceInfo = renderPass ? &inheritanceInfo : nullptr; // For secondary command buffers only
    vkBeginCommandBuffer(CommandBuffer, &beginInfo);
};
inline void
RIRenderCommandListVk::End()
{
    // Must be in recording state
    check(IsRecording());

    vkEndCommandBuffer(CommandBuffer);

    _recordingState = 0;
    WasRecorded     = true;
};

inline void
RIRenderCommandListVk::BeginRenderPass(RIRenderPass* renderpass, RIFramebuffer* framebuffer, bool recordSecondaryBuffer, const DLinearColor& clearColor)
{
    // Must be in recording state
    check(IsRecording());
    check(!IsRenderPassBound());
    isRenderPassBound = true;

    RIRenderPassVk* renderPassVk = static_cast<RIRenderPassVk*>(renderpass);
    check(renderPassVk);

    RIFramebufferVk* fboVk = static_cast<RIFramebufferVk*>(framebuffer);
    check(fboVk);

    // Setup clear values
    const VkClearColorValue        clearColorValue        = { clearColor.RGBA[0], clearColor.RGBA[1], clearColor.RGBA[2], clearColor.RGBA[3] };
    const VkClearDepthStencilValue clearDepthStencilValue = { 1.f, 0 };
    VkClearValue                   clearValues[2];
    clearValues[0].color        = clearColorValue;
    clearValues[1].depthStencil = clearDepthStencilValue;

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = renderPassVk->RenderPass;
    renderPassInfo.framebuffer       = fboVk->Framebuffer;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = { fboVk->Width, fboVk->Height };
    renderPassInfo.clearValueCount   = 2;
    renderPassInfo.pClearValues      = clearValues;

    vkCmdBeginRenderPass(CommandBuffer, &renderPassInfo, !recordSecondaryBuffer ? VK_SUBPASS_CONTENTS_INLINE : VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
}

inline void
RIRenderCommandListVk::EndRenderPass()
{
    check(IsRecording());
    check(IsRenderPassBound());
    isRenderPassBound = false;
    vkCmdEndRenderPass(CommandBuffer);
};
inline void
RIRenderCommandListVk::BindGraphicsPipeline(RIPipeline* pipeline)
{
    // Must be in recording state
    check(IsReadyForDrawingCommand());

    RIPipelineVk* ripso = static_cast<RIPipelineVk*>(pipeline);
    check(ripso);
    vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ripso->Pipeline);
};
inline void
RIRenderCommandListVk::BindVertexBuffer(RIVertexBuffer* buffer)
{
    // Must be in recording state
    check(IsReadyForDrawingCommand());

    RIVertexBufferVk* vkbuf = static_cast<RIVertexBufferVk*>(buffer);
    check(vkbuf);
    // TODO: Vertex buffer is mono buffered
    std::vector<VkBuffer> buffers   = { vkbuf->Buffer.Buffers[0] };
    VkDeviceSize          offsets[] = { 0 };

    vkCmdBindVertexBuffers(CommandBuffer, 0, (uint32_t)buffers.size(), buffers.data(), offsets);
};
inline void
RIRenderCommandListVk::Draw(size_t vertexCount, size_t instanceCount, size_t firstVertex, size_t firstInstance)
{
    // Must be in recording state
    check(IsReadyForDrawingCommand());

    vkCmdDraw(CommandBuffer, (uint32_t)vertexCount, (uint32_t)instanceCount, (uint32_t)firstVertex, (uint32_t)firstInstance);
}
inline void
RIRenderCommandListVk::SetViewport(const DViewport& viewport)
{
    // Must be in recording state
    check(IsReadyForDrawingCommand());

    VkViewport vp = {};
    vp.x          = viewport.X;
    vp.y          = viewport.Y;
    vp.width      = viewport.Width;
    vp.height     = viewport.Height;
    vp.minDepth   = viewport.Znear;
    vp.maxDepth   = viewport.Zfar;

    check(vp.minDepth < 1.f);
    check(vp.maxDepth > 0.f);

    vkCmdSetViewport(CommandBuffer, 0, 1, &vp);
};

inline void
RIRenderCommandListVk::SetScissor(const DViewport& viewport)
{
    // Must be in recording state
    check(IsReadyForDrawingCommand());

    VkRect2D vp = { { (int32_t)viewport.X, (int32_t)viewport.Y }, { (uint32_t)viewport.Width, (uint32_t)viewport.Height } };

    vkCmdSetScissor(CommandBuffer, 0, 1, &vp);
}

inline void
RIRenderCommandListVk::PushConstants(RIPipeline* pipeline, size_t size, void* data)
{
    // Must be in recording state
    check(IsReadyForDrawingCommand());

    RIPipelineVk* vkPipeline = static_cast<RIPipelineVk*>(pipeline);
    check(vkPipeline);

    vkCmdPushConstants(CommandBuffer, vkPipeline->PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, (uint32_t)size, data);
};

inline void
RIRenderCommandListVk::BindDescriptorSets(uint32_t firstSet, std::vector<RIShaderDescriptorsSet*> descriptorSets, RIPipeline* pipeline)
{
    // Must be in recording state
    check(IsReadyForDrawingCommand());

    RIPipelineVk* vkPipeline = static_cast<RIPipelineVk*>(pipeline);
    check(vkPipeline);

    std::vector<VkDescriptorSet> vkdescriptorSetsList(descriptorSets.size(), nullptr);

    for (size_t i = 0; i < descriptorSets.size(); i++)
        {
            RIShaderDescriptorSetVk* riDescriptorSet = static_cast<RIShaderDescriptorSetVk*>(descriptorSets[i]);

            vkdescriptorSetsList[i] = riDescriptorSet->DescriptorSet;
        }

    vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline->PipelineLayout, 0, (uint32_t)vkdescriptorSetsList.size(), vkdescriptorSetsList.data(), 0, NULL);
};

inline void
RIRenderCommandListVk::ExecuteSecondaryCommandList(std::vector<RIRenderCommandList*> commandList)
{
    // Must be in recording state
    check(IsReadyForDrawingCommand());

    std::vector<VkCommandBuffer> vkCommands;
    vkCommands.resize(commandList.size());

    for (size_t i = 0; i < commandList.size(); i++)
        {
            auto* ricmd   = static_cast<RIRenderCommandListVk*>(commandList[i]);
            vkCommands[i] = ricmd->CommandBuffer;
        }

    vkCmdExecuteCommands(CommandBuffer, (uint32_t)commandList.size(), vkCommands.data());
};


}
#endif