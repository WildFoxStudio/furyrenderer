// Copyright RedFox Studio 2022

#pragma once

#include "Core/window/WindowBase.h"
#include "VulkanDevice.h"

#include <volk.h>

#include <unordered_set>

namespace Fox
{
// Swapchain implementation
class RIVulkanDevice2 : public RIVulkanDevice
{
    static constexpr int32_t MAX_IMAGE_COUNT = 2;

  public:
    virtual ~RIVulkanDevice2();

    bool                            SurfaceSupportPresentationOnCurrentQueueFamily(VkSurfaceKHR surface);
    std::vector<VkSurfaceFormatKHR> GetSurfaceFormats(VkSurfaceKHR surface);
    VkSurfaceCapabilitiesKHR        GetSurfaceCapabilities(VkSurfaceKHR surface);
    std::vector<VkPresentModeKHR>   GetSurfacePresentModes(VkSurfaceKHR surface);
    /**
     * @brief Creates a swapchain object
     * @param surface Valid surface
     * @param format Valid format supported by the surface
     * @param presentMode Valid present mode from the surface
     * @param capabilities Capabilities of the surface
     * @param oldSwapchain The old swapchain if exists, nullptr otherwise
     * @return
     */
    VkSwapchainKHR CreateSwapchainFromSurface(VkSurfaceKHR surface,
    VkSurfaceFormatKHR&                                    format,
    VkPresentModeKHR&                                      presentMode,
    VkSurfaceCapabilitiesKHR&                              capabilities,
    VkSwapchainKHR                                         oldSwapchain = nullptr);
    /**
     * @brief Must be externally synchronized
     * @param swapchain Swapchain to destroy
     */
    void DestroySwapchain(VkSwapchainKHR swapchain);

    std::vector<VkImage> GetSwapchainImages(VkSwapchainKHR swapchain);

    VkResult AcquireNextImage(VkSwapchainKHR swapchain, uint64_t timeoutNanoseconds, uint32_t* imageIndex, VkSemaphore imageAvailableSempahore, VkFence fence = VK_NULL_HANDLE);

    std::vector<VkResult> Present(std::vector<std::pair<VkSwapchainKHR, uint32_t>> swapchainImageIndex, std::vector<VkSemaphore> waitSemaphores);

  private:
    std::unordered_set<VkSwapchainKHR> _swapchains;
};
}