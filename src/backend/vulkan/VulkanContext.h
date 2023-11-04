// Copyright RedFox Studio 2022

#pragma once

#include "IContext.h"

#include "DescriptorPool.h"
#include "RingBufferManager.h"
#include "VulkanDevice13.h"
#include "VulkanInstance.h"

#include <array>
#include <functional>
#include <list>
#include <tuple>
#include <vector>

namespace Fox
{

struct DResource
{
    uint8_t Id{};
};

struct DBufferVulkan : public DResource
{
    uint32_t       Size{};
    RIVulkanBuffer Buffer;
};

struct DImageVulkan : public DResource
{
    RIVulkanImage      Image;
    VkImageView        View{};
    VkImageAspectFlags ImageAspect{};
    VkSampler          Sampler{};
};

struct DRenderTargetVulkan : public DResource
{
    RIVulkanImage      Image;
    VkImageView        View{};
    VkImageAspectFlags ImageAspect{};
};

struct DFramebufferVulkan : public DResource
{
    VkFramebuffer           Framebuffer{};
    uint32_t                Width{}, Height{};
    DFramebufferAttachments Attachments; // Used for comparing framebuffers
};

struct DSwapchainVulkan : public DResource
{
    static constexpr uint32_t             MAX_IMAGE_COUNT = 4;
    VkSurfaceKHR                          Surface{};
    VkSurfaceCapabilitiesKHR              Capabilities;
    VkSurfaceFormatKHR                    Format;
    VkPresentModeKHR                      PresentMode;
    VkSwapchainKHR                        Swapchain{};
    uint32_t                              ImagesCount{};
    std::array<uint32_t, MAX_IMAGE_COUNT> ImagesId;
    uint32_t                              RenderTargetsId[MAX_IMAGE_COUNT]{};
};

struct DVertexInputLayoutVulkan : public DResource
{
    std::vector<VkVertexInputAttributeDescription> VertexInputAttributes;
};

struct DPipelineVulkan_DEPRECATED : public DPipeline_T
{
    VkPipeline           Pipeline{};
    VkPipelineLayout     PipelineLayout{};
    std::vector<EFormat> Attachments;
};

struct DPipelineVulkan : public DResource
{
    VkPipeline              Pipeline{};
    const VkPipelineLayout* PipelineLayout{};
};

struct DCommandPoolVulkan : public DResource
{
    VkCommandPool Pool{};
};

struct DCommandBufferVulkan : public DResource
{
    VkCommandBuffer Cmd{};
    bool            IsRecording{};
    VkRenderPass    ActiveRenderPass{};
};

struct DFenceVulkan : public DResource
{
    VkFence Fence{};
    bool    IsSignaled{};
};

struct DSemaphoreVulkan : public DResource
{
    VkSemaphore Semaphore{};
};

struct DShaderVulkan : public DResource
{
    VertexInputLayoutId                          VertexLayout{};
    uint32_t                                     VertexStride{};
    VkShaderModule                               VertexShaderModule{};
    VkShaderModule                               PixelShaderModule{};
    std::vector<VkPipelineShaderStageCreateInfo> ShaderStageCreateInfo;
    uint32_t                                     ColorAttachments{ 1 };
    bool                                         DepthStencilAttachment{};
};

struct DRootSignature : public DResource
{
    VkPipelineLayout PipelineLayout{};
    /*Since VkDescriptorSetLayout are cached it can be used by multiple shaders, be careful when deleting*/
    VkDescriptorSetLayout DescriptorSetLayouts[(uint32_t)EDescriptorFrequency::MAX_COUNT]{};
    // VkDescriptorPool                  EmptyPools[(uint32_t)EDescriptorFrequency::MAX_COUNT];
    // std::vector<VkDescriptorSet>      EmptyDescriptorSets[(uint32_t)EDescriptorFrequency::MAX_COUNT];
    std::vector<VkDescriptorPoolSize>                                PoolSizes[(uint32_t)EDescriptorFrequency::MAX_COUNT];
    std::map<uint32_t, std::map<uint32_t, ShaderDescriptorBindings>> SetsBindings;
    VkDescriptorPool                                                 EmptyPool[(uint32_t)EDescriptorFrequency::MAX_COUNT]{};
    VkDescriptorSet                                                  EmptySet[(uint32_t)EDescriptorFrequency::MAX_COUNT]{};
};

struct DDescriptorSet : public DResource
{
    VkDescriptorPool                             DescriptorPool;
    std::vector<VkDescriptorSet>                 Sets;
    EDescriptorFrequency                         Frequency;
    std::map<uint32_t, ShaderDescriptorBindings> Bindings;
    const DRootSignature*                        RootSignature{};
};

struct DSamplerVulkan : public DResource
{
    VkSampler Sampler{};
};

class VulkanContext final : public IContext
{
    inline static constexpr uint32_t NUM_OF_FRAMES_IN_FLIGHT{ 2 };

  public:
    VulkanContext(const DContextConfig* const config);
    ~VulkanContext();
    void                  WaitDeviceIdle() override;
    SwapchainId           CreateSwapchain(const WindowData* windowData, EPresentMode& presentMode, EFormat& outFormat) override;
    std::vector<uint32_t> GetSwapchainRenderTargets(SwapchainId swapchainId) override;
    bool                  SwapchainAcquireNextImageIndex(SwapchainId swapchainId, uint64_t timeoutNanoseconds, uint32_t sempahoreid, uint32_t* outImageIndex) override;
    void                  DestroySwapchain(SwapchainId swapchainId) override;

    BufferId            CreateBuffer(uint32_t size, EResourceType type, EMemoryUsage usage) override;
    void*               BeginMapBuffer(BufferId buffer) override;
    void                EndMapBuffer(BufferId buffer) override;
    void                DestroyBuffer(BufferId buffer) override;
    ImageId             CreateImage(EFormat format, uint32_t width, uint32_t height, uint32_t mipMapCount) override;
    EFormat             GetImageFormat(ImageId) const override;
    void                DestroyImage(ImageId imageId) override;
    VertexInputLayoutId CreateVertexLayout(const std::vector<VertexLayoutInfo>& info) override;
    ShaderId            CreateShader(const ShaderSource& source) override;
    void                DestroyShader(const ShaderId shader) override;
    uint32_t            CreateSampler(uint32_t minLod, uint32_t maxLod) override;

    uint32_t CreatePipeline(const ShaderId shader, uint32_t rootSignatureId, const DPipelineAttachments& attachments, const PipelineFormat& format) override;
    void     DestroyPipeline(uint32_t pipelineId) override;
    uint32_t CreateRootSignature(const ShaderLayout& layout) override;
    void     DestroyRootSignature(uint32_t rootSignatureId) override;
    uint32_t CreateDescriptorSets(uint32_t rootSignatureId, EDescriptorFrequency frequency, uint32_t count) override;
    void     DestroyDescriptorSet(uint32_t descriptorSetId) override;
    void     UpdateDescriptorSet(uint32_t descriptorSetId, uint32_t setIndex, uint32_t paramCount, DescriptorData* params) override;

    uint32_t CreateCommandPool() override;
    void     DestroyCommandPool(uint32_t commandPoolId) override;
    void     ResetCommandPool(uint32_t commandPoolId) override;

    uint32_t CreateCommandBuffer(uint32_t commandPoolId) override;
    void     DestroyCommandBuffer(uint32_t commandBufferId) override;
    void     BeginCommandBuffer(uint32_t commandBufferId) override;
    void     EndCommandBuffer(uint32_t commandBufferId) override;

    void BindRenderTargets(uint32_t commandBufferId, const DFramebufferAttachments& attachments, const DLoadOpPass& loadOP) override;
    void SetViewport(uint32_t commandBufferId, uint32_t x, uint32_t y, uint32_t width, uint32_t height, float znear, float zfar) override;
    void SetScissor(uint32_t commandBufferId, uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
    void BindPipeline(uint32_t commandBufferId, uint32_t pipeline) override;
    void BindVertexBuffer(uint32_t commandBufferId, uint32_t bufferId) override;
    void BindIndexBuffer(uint32_t commandBufferId, uint32_t bufferId) override;
    void Draw(uint32_t commandBufferId, uint32_t firstVertex, uint32_t count) override;
    void DrawIndexed(uint32_t commandBufferId, uint32_t index_count, uint32_t first_index, uint32_t first_vertex) override;
    void DrawIndexedIndirect(uint32_t commandBufferId, uint32_t buffer, uint32_t offset, uint32_t drawCount, uint32_t stride) override;
    void BindDescriptorSet(uint32_t commandBufferId, uint32_t setIndex, uint32_t descriptorSetId) override;
    void CopyImage(uint32_t commandId, uint32_t imageId, uint32_t width, uint32_t height, uint32_t mipMapIndex, uint32_t stagingBufferId, uint32_t stagingBufferOffset) override;

    uint32_t CreateFence(bool signaled) override;
    void     DestroyFence(uint32_t fenceId) override;

    bool IsFenceSignaled(uint32_t fenceId) override;
    void WaitForFence(uint32_t fenceId, uint64_t timeoutNanoseconds) override;
    void ResetFence(uint32_t fenceId) override;

    uint32_t CreateGpuSemaphore() override;
    void     DestroyGpuSemaphore(uint32_t semaphoreId) override;

    uint32_t CreateRenderTarget(EFormat format, ESampleBit samples, bool isDepth, uint32_t width, uint32_t height, uint32_t arrayLength, uint32_t mipMapCount, EResourceState initialState) override;
    void     DestroyRenderTarget(uint32_t renderTargetId) override;

    void ResourceBarrier(uint32_t commandBufferId,
    uint32_t                      buffer_barrier_count,
    BufferBarrier*                p_buffer_barriers,
    uint32_t                      texture_barrier_count,
    TextureBarrier*               p_texture_barriers,
    uint32_t                      rt_barrier_count,
    RenderTargetBarrier*          p_rt_barriers) override;

    void FlushDeletedBuffers() override;

    void QueueSubmit(const std::vector<uint32_t>& waitSemaphore, const std::vector<uint32_t>& finishSemaphore, const std::vector<uint32_t>& cmdIds, uint32_t fenceId) override;
    void QueuePresent(uint32_t swapchainId, uint32_t imageIndex, const std::vector<uint32_t>& waitSemaphore) override;

    unsigned char* GetAdapterDescription() const override;
    size_t         GetAdapterDedicatedVideoMemory() const override;

#pragma region Utility
    RIVulkanDevice13& GetDevice() { return Device; }
#pragma endregion

  private:
    static constexpr uint32_t MAX_RESOURCES     = 1024; // Change this number to increase number of allocations per resource
    static constexpr uint64_t MAX_FENCE_TIMEOUT = 0xffff; // 0xffffffffffffffff; // nanoseconds
    RIVulkanInstance          Instance;
    RIVulkanDevice13          Device;

    void (*_warningOutput)(const char*);
    void (*_logOutput)(const char*);

    DBufferVulkan                               _emptyUbo;
    uint32_t                                    _emptyImageId{};
    DImageVulkan*                               _emptyImage;
    DSamplerVulkan                              _emptySampler;
    std::array<DSwapchainVulkan, MAX_RESOURCES> _swapchains;
    std::array<DBufferVulkan, MAX_RESOURCES>    _vertexBuffers;
    std::array<DBufferVulkan, MAX_RESOURCES>    _transferBuffers;
    std::array<DBufferVulkan, MAX_RESOURCES>    _uniformBuffers;
    std::array<DBufferVulkan, MAX_RESOURCES>    _indirectBuffers;
    /*Whenever a render target gets deleted remove also framebuffers that have that image id as attachment*/
    std::array<DFramebufferVulkan, MAX_RESOURCES>       _framebuffers;
    std::array<DShaderVulkan, MAX_RESOURCES>            _shaders;
    std::array<DVertexInputLayoutVulkan, MAX_RESOURCES> _vertexLayouts;
    std::array<DImageVulkan, MAX_RESOURCES>             _images;
    std::array<DPipelineVulkan, MAX_RESOURCES>          _pipelines;
    std::array<DCommandPoolVulkan, MAX_RESOURCES>       _commandPools_DEPRECATED;
    std::array<DFenceVulkan, MAX_RESOURCES>             _fences;
    std::array<DSamplerVulkan, MAX_RESOURCES>           _samplers;
    std::array<DSemaphoreVulkan, MAX_RESOURCES>         _semaphores;
    std::array<DCommandPoolVulkan, MAX_RESOURCES>       _commandPools;
    std::array<DCommandBufferVulkan, MAX_RESOURCES>     _commandBuffers;
    std::array<DRenderTargetVulkan, MAX_RESOURCES>      _renderTargets;
    std::array<DRootSignature, MAX_RESOURCES>           _rootSignatures;
    std::array<DDescriptorSet, MAX_RESOURCES>           _descriptorSets;
    std::unordered_set<VkRenderPass>                    _renderPasses;

    using DeleteFn                 = std::function<void()>;
    using FramesWaitToDeletionList = std::pair<uint32_t, std::vector<DeleteFn>>;
    uint32_t                              _frameIndex{};
    std::vector<FramesWaitToDeletionList> _deletionQueue;

    // Staging buffer, used to copy stuff from ram to CPU-VISIBLE-MEMORY then to GPU-ONLY-MEMORY
    std::vector<std::vector<uint32_t>> _perFrameCopySizes;
    RIVulkanBuffer                     _stagingBuffer;
    std::unique_ptr<RingBufferManager> _stagingBufferManager;

    // Pipeline layout to map of descriptor set layout - used to retrieve the correct VkDescriptorSetLayout when creating descriptor set
    std::unordered_map<VkPipelineLayout, std::map<uint32_t, VkDescriptorSetLayout>> _pipelineLayoutToSetIndexDescriptorSetLayout;
    /*Per frame map of per pipeline layout to descriptor pool, we can allocate descriptor sets per pipeline layout*/
    std::vector<std::unordered_map<VkPipelineLayout, std::shared_ptr<RIDescriptorPoolManager>>> _pipelineLayoutToDescriptorPool;

    const std::vector<const char*> _validationLayers = {
        "VK_LAYER_KHRONOS_validation",
        "VK_LAYER_GOOGLE_threading",
        "VK_LAYER_LUNARG_parameter_validation",
        "VK_LAYER_LUNARG_object_tracker",
        "VK_LAYER_LUNARG_core_validation",
        "VK_LAYER_GOOGLE_unique_objects",
    };

#if FOX_PLATFORM == FOX_PLATFORM_WINDOWS32
    const std::vector<const char*> _instanceExtensionNames = { "VK_EXT_debug_utils",
        "VK_KHR_surface",
        "VK_KHR_win32_surface",
        "VK_KHR_external_semaphore_capabilities",
        VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME };
#elif FOX_PLATFORM == FOX_PLATFORM_LINUX
    const std::vector<const char*> _instanceExtensionNames = {
        "VK_EXT_debug_utils",
        "VK_KHR_surface",
        "VK_KHR_wayland_surface",
        "VK_KHR_xlib_surface",
        "VK_KHR_external_semaphore_capabilities",
        VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
    };
#else
#pragma error "Not supported"
#endif

    const std::vector<const char*> _deviceExtensionNames = { VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
        "VK_KHR_Maintenance1", // passing negative viewport heights
        "VK_KHR_maintenance4",
        "VK_KHR_dedicated_allocation",
        "VK_KHR_bind_memory2" };

    void Warning(const std::string& error);
    void Log(const std::string& error);

    void _initializeVolk();
    void _initializeInstance();
    void _initializeDebugger();
    void _initializeVersion();
    void _initializeDevice();
    void _initializeStagingBuffer(uint32_t stagingBufferSize);
    void _deinitializeStagingBuffer();

    uint32_t               _createFramebuffer(const DFramebufferAttachments& attachments);
    void                   _destroyFramebuffer(uint32_t framebufferId);
    DRenderPassAttachments _createGenericRenderPassAttachments(const DFramebufferAttachments& att);
    DRenderPassAttachments _createGenericRenderPassAttachmentsFromPipelineAttachments(const DPipelineAttachments& att);
    void                   _createSwapchain(DSwapchainVulkan& swapchain, const WindowData* windowData, EPresentMode& presentMode, EFormat& outFormat);
    uint32_t               _createImageFromVkImage(VkImage vkimage, VkFormat format, uint32_t width, uint32_t height);
    void                   _createVertexBuffer(uint32_t size, DBufferVulkan& buffer);
    void                   _createUniformBuffer(uint32_t size, DBufferVulkan& buffer);
    void                   _createShader(const ShaderSource& source, DShaderVulkan& shader);
    void                   _performDeletionQueue();
    void                   _deferDestruction(DeleteFn&& fn);
    VkPipeline             _createPipeline(VkPipelineLayout         pipelineLayout,
                VkRenderPass                                        renderPass,
                const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
                const PipelineFormat&                               format,
                const DVertexInputLayoutVulkan&                     vertexLayout,
                uint32_t                                            stride);
    void                   _recreateSwapchainBlocking(DSwapchainVulkan& swapchain);
    RIVkRenderPassInfo     _computeFramebufferAttachmentsRenderPassInfo(const std::vector<VkFormat>& attachmentFormat);

    static VKAPI_ATTR VkBool32 VKAPI_CALL _vulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT                                                                   messageType,
    const VkDebugUtilsMessengerCallbackDataEXT*                                                       pCallbackData,
    void*                                                                                             pUserData);
    VkRenderPass                          _createRenderPass(const DRenderPassAttachments& attachments);
    VkRenderPass                          _createRenderPassFromInfo(const RIVkRenderPassInfo& info);
    std::vector<const char*>              _getInstanceSupportedExtensions(const std::vector<const char*>& extentions);
    std::vector<const char*>              _getInstanceSupportedValidationLayers(const std::vector<const char*>& validationLayers);
    VkPhysicalDevice                      _queryBestPhysicalDevice();
    std::vector<const char*>              _getDeviceSupportedExtensions(VkPhysicalDevice physicalDevice, const std::vector<const char*>& extentions);
    std::vector<const char*>              _getDeviceSupportedValidationLayers(VkPhysicalDevice physicalDevice, const std::vector<const char*>& validationLayers);
};

RIVkRenderPassInfo ConvertRenderPassAttachmentsToRIVkRenderPassInfo(const DRenderPassAttachments& attachments);
}