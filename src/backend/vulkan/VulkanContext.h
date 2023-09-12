// Copyright RedFox Studio 2022

#pragma once

#include "IContext.h"

#include "VulkanDevice12.h"
#include "VulkanInstance.h"

#include <vector>

namespace Fox
{

class VulkanContext : public IContext
{
  public:
    VulkanContext(const DContextConfig* const config);
    ~VulkanContext();

    bool CreateSwapchain(const WindowData* windowData, const EPresentMode& presentMode, DSwapchain* swapchain) override;
    void DestroySwapchain(const DSwapchain swapchain) override;

    DFramebuffer CreateSwapchainFramebuffer() override;
    void         DestroyFramebuffer(DFramebuffer framebuffer) override;

    void SubmitPass(RenderPassData&& data) override;
    void SubmitCopy(CopyDataCommand&& data) override;
    void AdvanceFrame() override;

    unsigned char* GetAdapterDescription() const override;
    size_t         GetAdapterDedicatedVideoMemory() const override;

  private:
    RIVulkanInstance Instance;
    RIVulkanDevice12 Device;

    void (*_warningOutput)(const char*);
    void (*_logOutput)(const char*);


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

    void OutWarning(const std::string& error);
    void Log(const std::string& error);

    void _initializeVolk();
    void _initializeInstance();
    void _initializeDebugger();
    void _initializeVersion();
    void _initializeDevice();

    std::vector<const char*> _getInstanceSupportedExtensions(const std::vector<const char*>& extentions);
    std::vector<const char*> _getInstanceSupportedValidationLayers(const std::vector<const char*>& validationLayers);
    VkPhysicalDevice         _queryBestPhysicalDevice();
    std::vector<const char*> _getDeviceSupportedExtensions(VkPhysicalDevice physicalDevice, const std::vector<const char*>& extentions);
    std::vector<const char*> _getDeviceSupportedValidationLayers(VkPhysicalDevice physicalDevice, const std::vector<const char*>& validationLayers);
};

}