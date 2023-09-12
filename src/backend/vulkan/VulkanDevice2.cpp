// Copyright RedFox Studio 2022

#include "VulkanDevice2.h"

#include "UtilsVK.h"
#include "asserts.h"

#include <algorithm>

namespace Fox
{

RIVulkanDevice2::~RIVulkanDevice2() { check(_swapchains.size() == 0); }

bool
RIVulkanDevice2::SurfaceSupportPresentationOnCurrentQueueFamily(VkSurfaceKHR surface)
{
    uint32_t supportPresentation = VK_FALSE;
    // To determine whether a queue family of a physical device supports presentation to a given surface
    const VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, GetQueueFamilyIndex(), surface, &supportPresentation);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkUtils::VkErrorString(result));
        }
    return supportPresentation == VK_TRUE ? true : false;
}

std::vector<VkSurfaceFormatKHR>
RIVulkanDevice2::GetSurfaceFormats(VkSurfaceKHR surface)
{
    std::vector<VkSurfaceFormatKHR> formats;
    {
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, surface, &formatCount, nullptr);

        if (formatCount != 0)
            {
                formats.resize(formatCount);
                const VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, surface, &formatCount, formats.data());
                if (VKFAILED(result))
                    {
                        throw std::runtime_error(VkUtils::VkErrorString(result));
                    }
            }
    }
    check(0);
    return {};
}

VkSurfaceCapabilitiesKHR
RIVulkanDevice2::GetSurfaceCapabilities(VkSurfaceKHR surface)
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
    {
        const VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, surface, &surfaceCapabilities);
        if (VKFAILED(result))
            {
                throw std::runtime_error(VkUtils::VkErrorString(result));
            }
    }

    return surfaceCapabilities;
}

std::vector<VkPresentModeKHR>
RIVulkanDevice2::GetSurfacePresentModes(VkSurfaceKHR surface)
{
    std::vector<VkPresentModeKHR> presentModes;
    {
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0)
            {
                presentModes.resize(presentModeCount);
                const VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, surface, &presentModeCount, presentModes.data());
                if (VKFAILED(result))
                    {
                        throw std::runtime_error(VkUtils::VkErrorString(result));
                    }
            }
    }
    return presentModes;
}

VkResult
RIVulkanDevice2::CreateSwapchainFromSurface(VkSurfaceKHR surface,
const VkSurfaceFormatKHR&                                format,
const VkPresentModeKHR&                                  presentMode,
const VkSurfaceCapabilitiesKHR&                          capabilities,
VkSwapchainKHR*                                          outSwapchain,
VkSwapchainKHR                                           oldSwapchain)
{
    uint32_t queueFamilyIndices[] = { (uint32_t)GetQueueFamilyIndex() };
    critical(capabilities.minImageCount >= MAX_IMAGE_COUNT);

    VkSwapchainCreateInfoKHR swapchainInfo = {};
    swapchainInfo.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.pNext                    = NULL;
    swapchainInfo.flags                    = 0;
    swapchainInfo.surface                  = surface;
    swapchainInfo.minImageCount            = MAX_IMAGE_COUNT;
    swapchainInfo.imageFormat              = format.format;
    swapchainInfo.imageColorSpace          = format.colorSpace;

    check(capabilities.currentExtent.width <= capabilities.maxImageExtent.width);
    check(capabilities.currentExtent.height <= capabilities.maxImageExtent.height);

    swapchainInfo.imageExtent           = capabilities.currentExtent;
    swapchainInfo.imageArrayLayers      = capabilities.maxImageArrayLayers;
    swapchainInfo.imageUsage            = capabilities.supportedUsageFlags; // VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;;
    swapchainInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.queueFamilyIndexCount = 1;
    swapchainInfo.pQueueFamilyIndices   = queueFamilyIndices;
    swapchainInfo.preTransform          = capabilities.currentTransform;
    swapchainInfo.compositeAlpha        = VkCompositeAlphaFlagBitsKHR::VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode           = presentMode;
    swapchainInfo.clipped               = 0;
    swapchainInfo.oldSwapchain          = oldSwapchain;

    const VkResult result = vkCreateSwapchainKHR(Device, &swapchainInfo, NULL, outSwapchain);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkUtils::VkErrorString(result));
        }

    _swapchains.insert(*outSwapchain);

    return result;
}

void
RIVulkanDevice2::DestroySwapchain(VkSwapchainKHR swapchain)
{
    vkDestroySwapchainKHR(Device, swapchain, nullptr);

    _swapchains.erase(_swapchains.find(swapchain));
}

std::vector<VkImage>
RIVulkanDevice2::GetSwapchainImages(VkSwapchainKHR swapchain)
{
    std::vector<VkImage> images;
    uint32_t             imageCount = 0;
    vkGetSwapchainImagesKHR(Device, swapchain, &imageCount, nullptr);

    check(imageCount >= MAX_IMAGE_COUNT);

    images.resize(imageCount);
    vkGetSwapchainImagesKHR(Device, swapchain, &imageCount, images.data());

    return images;
}

VkResult
RIVulkanDevice2::AcquireNextImage(VkSwapchainKHR swapchain, uint64_t timeoutNanoseconds, uint32_t* imageIndex, VkSemaphore imageAvailableSempahore, VkFence fence)
{
    // On success,
    // this command returns VK_SUCCESS

    // VK_TIMEOUT

    // VK_NOT_READY

    // VK_SUBOPTIMAL_KHR

    // On failure,
    // this command returns VK_ERROR_OUT_OF_HOST_MEMORY

    // VK_ERROR_OUT_OF_DEVICE_MEMORY

    // VK_ERROR_DEVICE_LOST

    // VK_ERROR_OUT_OF_DATE_KHR

    // VK_ERROR_SURFACE_LOST_KHR

    // VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT

    const VkResult result = vkAcquireNextImageKHR(Device, swapchain, timeoutNanoseconds, imageAvailableSempahore, fence, imageIndex);
    return result;
}

std::vector<VkResult>
RIVulkanDevice2::Present(std::vector<std::pair<VkSwapchainKHR, uint32_t>> swapchainImageIndex, std::vector<VkSemaphore> waitSemaphores)
{

    std::vector<VkResult> results(swapchainImageIndex.size(), VK_SUCCESS);
    check(results.size() == swapchainImageIndex.size());

    std::vector<VkSwapchainKHR> swapchains(swapchainImageIndex.size());
    std::vector<uint32_t>       imageIndices(swapchainImageIndex.size());

    std::transform(swapchainImageIndex.begin(), swapchainImageIndex.end(), swapchains.begin(), [](const std::pair<VkSwapchainKHR, uint32_t>& p) { return p.first; });
    std::transform(swapchainImageIndex.begin(), swapchainImageIndex.end(), imageIndices.begin(), [](const std::pair<VkSwapchainKHR, uint32_t>& p) { return p.second; });

    check(swapchains.size() == swapchainImageIndex.size());
    check(swapchains.size() == imageIndices.size());

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = (uint32_t)waitSemaphores.size();
    presentInfo.pWaitSemaphores    = waitSemaphores.data();
    presentInfo.swapchainCount     = (uint32_t)swapchainImageIndex.size();
    presentInfo.pSwapchains        = swapchains.data();
    presentInfo.pImageIndices      = imageIndices.data();
    presentInfo.pResults           = results.data();

    {
        [[maybe_unused]] const VkResult result = vkQueuePresentKHR(MainQueue, &presentInfo);
    }
    return results;
}

}