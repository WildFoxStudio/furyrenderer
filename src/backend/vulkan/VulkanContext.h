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

struct DFramebufferVulkan_DEPRECATED : public DResource
{
    /*Per frame or single framebuffer*/
    std::vector<VkFramebuffer> Framebuffers;
    uint32_t                   Width{}, Height{};
    DRenderPassAttachments     Attachments;
    RIVkRenderPassInfo         RenderPassInfo;
};

struct DFramebufferVulkan : public DResource
{
    VkFramebuffer Framebuffer{};
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
    // Deprecated below
    std::vector<VkImage>     Images;
    std::vector<VkImageView> ImageViews;
    FramebufferId            Framebuffers{};
    std::vector<VkSemaphore> ImageAvailableSemaphore;
    uint32_t                 CurrentImageIndex{};
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

struct DPipelinePermutations
{
    std::unordered_map<PipelineFormat, VkPipeline, PipelineFormatHashFn, PipelineFormatEqualFn> Pipeline{};
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
    /*Since VkDescriptorSetLayout are cached it can be used by multiple shaders, be careful when deleting*/
    std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;
    /*Since pipeline layouts are cached it can be used by multiple shaders, be careful when deleting*/
    VkPipelineLayout PipelineLayout{};

    // Hash to pipeline map
    std::unordered_map<std::vector<DRenderPassAttachment>, DPipelinePermutations, DRenderPassAttachmentsFormatsOnlyHashFn, DRenderPassAttachmentsFormatsOnlyEqualFn>
    RenderPassFormatToPipelinePermutationMap;
};

class VulkanContext final : public IContext
{
    inline static constexpr uint32_t NUM_OF_FRAMES_IN_FLIGHT{ 2 };

  public:
    VulkanContext(const DContextConfig* const config);
    ~VulkanContext();
    void                 WaitDeviceIdle() override;
    SwapchainId          CreateSwapchain(const WindowData* windowData, EPresentMode& presentMode, EFormat& outFormat) override;
    std::vector<ImageId> GetSwapchainImages(SwapchainId swapchainId) override;
    void                 DestroySwapchain(SwapchainId swapchainId) override;

    FramebufferId CreateSwapchainFramebuffer_DEPRECATED(SwapchainId swapchainId) override;
    void          DestroyFramebuffer_DEPRECATED(FramebufferId framebufferId) override;

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

    uint32_t CreatePipeline(const ShaderId shader, const DFramebufferAttachments& attachments, const PipelineFormat& format) override;
    void     DestroyPipeline(uint32_t pipelineId) override;

    uint32_t CreateFramebuffer(const DFramebufferAttachments& attachments) override;
    void     DestroyFramebuffer(uint32_t framebufferId) override;

    void SubmitPass(RenderPassData&& data) override;
    void SubmitCopy(CopyDataCommand&& data) override;
    void AdvanceFrame() override;
    void FlushDeletedBuffers() override;

    unsigned char* GetAdapterDescription() const override;
    size_t         GetAdapterDedicatedVideoMemory() const override;

#pragma region Utility
    RIVulkanDevice13& GetDevice() { return Device; }
#pragma endregion

  private:
    static constexpr uint32_t MAX_RESOURCES     = 4096;
    static constexpr uint64_t MAX_FENCE_TIMEOUT = 0xffff; // 0xffffffffffffffff; // nanoseconds
    RIVulkanInstance          Instance;
    RIVulkanDevice13          Device;

    void (*_warningOutput)(const char*);
    void (*_logOutput)(const char*);

    std::array<DSwapchainVulkan, MAX_RESOURCES>              _swapchains;
    std::array<DBufferVulkan, MAX_RESOURCES>                 _vertexBuffers;
    std::array<DBufferVulkan, MAX_RESOURCES>                 _uniformBuffers;
    std::array<DFramebufferVulkan_DEPRECATED, MAX_RESOURCES> _framebuffers_DEPRECATED;
    std::array<DFramebufferVulkan, MAX_RESOURCES>            _framebuffers;
    std::array<DShaderVulkan, MAX_RESOURCES>                 _shaders;
    std::array<DVertexInputLayoutVulkan, MAX_RESOURCES>      _vertexLayouts;
    std::array<DImageVulkan, MAX_RESOURCES>                  _images;
    std::array<DPipelineVulkan, MAX_RESOURCES>               _pipelines;
    std::vector<DFramebufferVulkan>                          _transientFramebuffers;

    std::unordered_set<VkRenderPass> _renderPasses;
    using DeleteFn                 = std::function<void()>;
    using FramesWaitToDeletionList = std::pair<uint32_t, std::vector<DeleteFn>>;
    uint32_t                                                                   _frameIndex{};
    std::vector<FramesWaitToDeletionList>                                      _deletionQueue;
    std::array<std::unique_ptr<RICommandPoolManager>, NUM_OF_FRAMES_IN_FLIGHT> _cmdPool;
    std::array<VkSemaphore, NUM_OF_FRAMES_IN_FLIGHT>                           _workFinishedSemaphores;
    std::array<VkFence, NUM_OF_FRAMES_IN_FLIGHT>                               _fence;
    std::vector<CopyDataCommand>                                               _transferCommands;
    std::vector<RenderPassData>                                                _drawCommands;
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
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME };
#elif FOX_PLATFORM == FOX_PLATFORM_LINUX
    const std::vector<const char*> _instanceExtensionNames = {
        "VK_EXT_debug_utils",
        "VK_KHR_surface",
        "VK_KHR_wayland_surface",
        "VK_KHR_xlib_surface",
        "VK_KHR_external_semaphore_capabilities",
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

    DRenderPassAttachments _createGenericRenderPassAttachments(const DFramebufferAttachments& att);
    void                   _createSwapchain(DSwapchainVulkan& swapchain, const WindowData* windowData, EPresentMode& presentMode, EFormat& outFormat);
    uint32_t               _createImageFromVkImage(VkImage vkimage, VkFormat format, uint32_t width, uint32_t height);
    void                   _createVertexBuffer(uint32_t size, DBufferVulkan& buffer);
    void                   _createUniformBuffer(uint32_t size, DBufferVulkan& buffer);
    void                   _createShader(const ShaderSource& source, DShaderVulkan& shader);
    void                   _createFramebufferFromSwapchain(DSwapchainVulkan& swapchain);
    void                   _performDeletionQueue();
    void                   _performCopyOperations();
    void                   _submitCommands(const std::vector<VkSemaphore>& imageAvailableSemaphores);
    void                   _deferDestruction(DeleteFn&& fn);
    VkPipeline             _createPipeline(VkPipelineLayout         pipelineLayout,
                VkRenderPass                                        renderPass,
                const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
                const PipelineFormat&                               format,
                const DVertexInputLayoutVulkan&                     vertexLayout,
                uint32_t                                            stride);
    void                   _recreateSwapchainBlocking(DSwapchainVulkan& swapchain);
    RIVkRenderPassInfo     _computeFramebufferAttachmentsRenderPassInfo(const std::vector<VkFormat>& attachmentFormat);
    VkPipeline             _queryPipelineFromAttachmentsAndFormat(DShaderVulkan& shader, const DRenderPassAttachments& renderPass, const PipelineFormat& format);

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