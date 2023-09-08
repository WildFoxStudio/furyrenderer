// Copyright RedFox Studio 2022

#pragma once

#if defined(FOX_USE_VULKAN)

#include "Core/platform/Fox_API.h"
#include "RIFactoryVk.h"

namespace Fox
{

class RIContextVk final : public RIFactoryVk
{
    inline static constexpr uint32_t STAGING_BUFFER_SIZE = 64 * 1024 * 1024; // 64mb

  public:
    RIContextVk();
    virtual ~RIContextVk();

    ERI_ERROR TryAcquireNextImage(RISwapchain* swapchain, uint32_t* frameIndex, bool& wasRecreated);

    void WaitGpuIdle() const override;

#pragma region FrontEnd rendering commands
    bool       CreateSwapchain(AWindow* window, VkSurfaceFormatKHR format, VkPresentModeKHR presentMode);
    void       DestroySwapchain(WindowIdentifier id);
    VkExtent2D GetSwapchainExtent(WindowIdentifier id);

    FramebufferRef GetSwapchainFramebuffer(WindowIdentifier id);

    void SubmitPass(RenderPassData&& data);
    /* Copy operations that don't fit inside the staging buffer will be executed on the next frame in the same order of submission*/
    void SubmitCopy(CopyDataCommand&& data);
    void AdvanceFrame();
#pragma endregion

  protected:
  private:
    static constexpr uint64_t MAX_FENCE_TIMEOUT       = 0xffffffffffffffff; // nanoseconds
    static constexpr uint32_t VERTEX_INDEX_POOL_BYTES = 256 * 1024 * 1024; // nanoseconds
    static constexpr uint32_t UBO_SSBO_POOL_BYTES     = 256 * 1024 * 1024; // nanoseconds
    uint32_t                  _frameIndex{};

    std::vector<RICommandPoolManager>                 _cmdPool;
    std::vector<RenderPassData>                       _renderPasses;
    std::unordered_map<WindowIdentifier, SwapchainVk> _windowToSwapchain;
    std::vector<VkSemaphore>                          _workFinishedSemaphores; // per frame semaphore
    std::vector<VkFence>                              _fence; // per frame fence
    std::vector<CopyDataCommand>                      _transferCommands;
    uint32_t                                          _stagingBufferManagerSetTailOnNextFrame{};//This is used to move tail (free space) of previous frame data

    void _waitFence(VkFence fence);
    /*Swapchain resource must not be in use when calling this function*/
    void _recreateSwapchain(SwapchainVk& swapchain);

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

    inline std::vector<const char*> _getInstanceSupportedExtensions(const std::vector<const char*>& extentions);
    inline std::vector<const char*> _getInstanceSupportedValidationLayers(const std::vector<const char*>& validationLayers);
    inline void                     _printVersion();
    inline VkPhysicalDevice         _queryBestPhysicalDevice();
    inline std::vector<const char*> _getDeviceSupportedExtensions(VkPhysicalDevice physicalDevice, const std::vector<const char*>& extentions);
    inline std::vector<const char*> _getDeviceSupportedValidationLayers(VkPhysicalDevice physicalDevice, const std::vector<const char*>& validationLayers);
};

} // namespace Fox

#include "RIContextVk.inl"

#endif