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

struct DResourceId
{
    uint8_t Id;
};

struct DFramebufferVulkan : public DFramebuffer_T
{
    /*Per frame or single framebuffer*/
    std::vector<VkFramebuffer> Framebuffers;
    uint32_t                   Width, Height;
    RIVkRenderPassInfo         RenderPassInfo;
};

struct DSwapchainVulkan : public DSwapchain_T
{
    VkSurfaceKHR             Surface;
    VkSurfaceCapabilitiesKHR Capabilities;
    VkSurfaceFormatKHR       Format;
    VkPresentModeKHR         PresentMode;
    VkSwapchainKHR           Swapchain{};
    std::vector<VkImage>     Images;
    std::vector<VkImageView> ImageViews;
    DFramebufferVulkan*      Framebuffers;
    // std::vector<VkFramebuffer> Framebuffers;
    std::vector<VkSemaphore> ImageAvailableSemaphore;
    uint32_t                 CurrentImageIndex{};
};

struct DBufferVulkan : public DBuffer_T
{
    DBufferVulkan(EBufferType type, uint32_t size) : DBuffer_T{ type }, Size(size){};
    const uint32_t Size;
    RIVulkanBuffer Buffer;
};

struct DImageVulkan : public DImage_T
{
    RIVulkanImage Image;
    VkImageView   View{};
    VkSampler     Sampler{};
};

struct DVertexInputLayoutVulkan : public DResourceId
{
    std::vector<VkVertexInputAttributeDescription> VertexInputAttributes;
};

struct DPipelineVulkan : public DPipeline_T
{
    VkPipeline           Pipeline{};
    VkPipelineLayout     PipelineLayout{};
    std::vector<EFormat> Attachments;
};

struct DPipelineAndLayoutVulkan
{
    VkPipeline       Pipeline{};
    VkPipelineLayout PipelineLayout{};
};

struct DShaderVulkan : public DResourceId
{
    VertexInputLayoutId                          VertexLayout{};
    uint32_t                                     VertexStride{};
    VkShaderModule                               VertexShaderModule{};
    VkShaderModule                               PixelShaderModule{};
    std::vector<VkPipelineShaderStageCreateInfo> ShaderStageCreateInfo;
    DRenderPassAttachments                       RenderPassAttachments;
    /*Since VkDescriptorSetLayout are cached it can be used by multiple shaders, be careful when deleting*/
    std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;
    /*Since pipeline layouts are cached it can be used by multiple shaders, be careful when deleting*/
    VkPipelineLayout PipelineLayout{};

    // Hash to pipeline map
    VkPipeline Pipelines;
};

class VulkanContext final : public IContext
{
    inline static constexpr uint32_t NUM_OF_FRAMES_IN_FLIGHT{ 2 };

  public:
    VulkanContext(const DContextConfig* const config);
    ~VulkanContext();

    bool CreateSwapchain(const WindowData* windowData, EPresentMode& presentMode, EFormat& outFormat, DSwapchain* swapchain) override;
    void DestroySwapchain(const DSwapchain swapchain) override;

    DFramebuffer CreateSwapchainFramebuffer(DSwapchain swapchain, DImage depth = nullptr) override;
    void         DestroyFramebuffer(DFramebuffer framebuffer) override;

    DBuffer             CreateVertexBuffer(uint32_t size) override;
    void                DestroyVertexBuffer(DBuffer buffer) override;
    DBuffer             CreateUniformBuffer(uint32_t size) override;
    void                DestroyUniformBuffer(DBuffer buffer) override;
    DImage              CreateImage(EFormat format, uint32_t width, uint32_t height, uint32_t mipMapCount) override;
    void                DestroyImage(DImage image) override;
    VertexInputLayoutId CreateVertexLayout(const std::vector<VertexLayoutInfo>& info) override;
    ShaderId            CreateShader(const ShaderSource& source) override;
    void                DestroyShader(const ShaderId shader) override;
    DPipeline           CreatePipeline(const ShaderSource& shader, const PipelineFormat& format, const std::vector<EFormat>& attachments);
    void                DestroyPipeline(DPipeline pipeline);

    void SubmitPass(RenderPassData&& data) override;
    void SubmitCopy(CopyDataCommand&& data) override;
    void AdvanceFrame() override;
    void FlushDeletedBuffers() override;

    unsigned char* GetAdapterDescription() const override;
    size_t         GetAdapterDedicatedVideoMemory() const override;

#pragma region Utility
    void              WaitDeviceIdle();
    RIVulkanDevice13& GetDevice() { return Device; }
#pragma endregion

  private:
    static constexpr uint32_t MAX_RESOURCES     = 4096;
    static constexpr uint64_t MAX_FENCE_TIMEOUT = 0xffff; // 0xffffffffffffffff; // nanoseconds
    RIVulkanInstance          Instance;
    RIVulkanDevice13          Device;

    void (*_warningOutput)(const char*);
    void (*_logOutput)(const char*);

    std::list<DSwapchainVulkan>      _swapchains;
    std::list<DBufferVulkan>         _vertexBuffers;
    std::list<DBufferVulkan>         _uniformBuffers;
    std::list<DImageVulkan>          _images;
    std::unordered_set<VkRenderPass> _renderPasses;
    using DeleteFn                 = std::function<void()>;
    using FramesWaitToDeletionList = std::pair<uint32_t, std::vector<DeleteFn>>;
    uint32_t                              _frameIndex{};
    std::vector<FramesWaitToDeletionList> _deletionQueue;
    std::vector<RICommandPoolManager>     _cmdPool;
    std::vector<VkSemaphore>              _workFinishedSemaphores;
    std::vector<VkFence>                  _fence;
    std::vector<CopyDataCommand>          _transferCommands;
    std::vector<RenderPassData>           _drawCommands;
    // Staging buffer, used to copy stuff from ram to CPU-VISIBLE-MEMORY then to GPU-ONLY-MEMORY
    std::vector<std::vector<uint32_t>> _perFrameCopySizes;
    RIVulkanBuffer                     _stagingBuffer;
    std::unique_ptr<RingBufferManager> _stagingBufferManager;
    // No need to delete them, are POD structs
    std::vector<DVertexInputLayoutVulkan> _vertexLayouts;
    // Framebuffers
    std::list<DFramebufferVulkan> _framebuffers;
    // Shader list
    std::array<DShaderVulkan, 4096> _shaders;

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

    /*generate a random number identitier to be used for resources*/
    uint8_t _genIdentifier();
    size_t  _findFirstFreeShaderIndex();


    void               _createShader(const ShaderSource& source, DShaderVulkan& shader);
    void               _performDeletionQueue();
    void               _performCopyOperations();
    void               _submitCommands(const std::vector<VkSemaphore>& imageAvailableSemaphores);
    void               _deferDestruction(DeleteFn&& fn);
    VkPipeline         _createPipeline(VkPipelineLayout pipelineLayout, VkRenderPass renderPass, const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages, const PipelineFormat& format);
    void               _recreateSwapchainBlocking(DSwapchainVulkan* swapchain);
    RIVkRenderPassInfo _computeFramebufferAttachmentsRenderPassInfo(const std::vector<VkFormat>& attachmentFormat);

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