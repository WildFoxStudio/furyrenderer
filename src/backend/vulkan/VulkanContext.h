// Copyright RedFox Studio 2022

#pragma once

#include "IContext.h"

#include "RingBufferManager.h"
#include "VulkanDevice13.h"
#include "VulkanInstance.h"

#include <functional>
#include <list>
#include <tuple>
#include <vector>

namespace Fox
{

struct DSwapchainVulkan : public DSwapchain_T
{
    VkSurfaceKHR               Surface;
    VkSurfaceCapabilitiesKHR   Capabilities;
    VkSurfaceFormatKHR         Format;
    VkSwapchainKHR             Swaphchain{};
    std::vector<VkImage>       Images;
    std::vector<VkImageView>   ImageViews;
    std::vector<VkFramebuffer> Framebuffers;
    std::vector<VkSemaphore>   ImageAvailableSemaphore;
    uint32_t                   CurrentImageIndex{};
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
};

class VulkanContext final : public IContext
{
    inline static constexpr uint32_t NUM_OF_FRAMES_IN_FLIGHT{ 2 };

  public:
    VulkanContext(const DContextConfig* const config);
    ~VulkanContext();

    bool CreateSwapchain(const WindowData* windowData, EPresentMode& presentMode, EFormat& outFormat, DSwapchain* swapchain) override;
    void DestroySwapchain(const DSwapchain swapchain) override;

    DFramebuffer CreateSwapchainFramebuffer() override;
    void         DestroyFramebuffer(DFramebuffer framebuffer) override;

    DBuffer CreateVertexBuffer(uint32_t size);
    void    DestroyVertexBuffer(DBuffer buffer);
    DBuffer CreateUniformBuffer(uint32_t size) override;
    void    DestroyUniformBuffer(DBuffer buffer) override;
    DImage  CreateImage(EFormat format, uint32_t width, uint32_t height, uint32_t mipMapCount);
    void    DestroyImage(DImage image);

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
    RIVulkanInstance Instance;
    RIVulkanDevice13 Device;

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
    std::vector<CopyDataCommand>          _transferCommands;
    // Staging buffer
    std::vector<std::vector<uint32_t>> _perFrameCopySizes;
    RIVulkanBuffer                     _stagingBuffer;
    std::unique_ptr<RingBufferManager> _stagingBufferManager;

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

    void _performDeletionQueue();
    void _performCopyOperations();
    void _submitCommands();
    void _deferDestruction(DeleteFn&& fn);

    static VKAPI_ATTR VkBool32 VKAPI_CALL _vulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT                                                                   messageType,
    const VkDebugUtilsMessengerCallbackDataEXT*                                                       pCallbackData,
    void*                                                                                             pUserData);
    VkRenderPass                          _createRenderPass(const DRenderPassAttachments& attachments);
    std::vector<const char*>              _getInstanceSupportedExtensions(const std::vector<const char*>& extentions);
    std::vector<const char*>              _getInstanceSupportedValidationLayers(const std::vector<const char*>& validationLayers);
    VkPhysicalDevice                      _queryBestPhysicalDevice();
    std::vector<const char*>              _getDeviceSupportedExtensions(VkPhysicalDevice physicalDevice, const std::vector<const char*>& extentions);
    std::vector<const char*>              _getDeviceSupportedValidationLayers(VkPhysicalDevice physicalDevice, const std::vector<const char*>& validationLayers);
};

RIVkRenderPassInfo ConvertRenderPassAttachmentsToRIVkRenderPassInfo(const DRenderPassAttachments& attachments);
}