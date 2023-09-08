// Copyright RedFox Studio 2022

#pragma once

#include "RenderInterface/vulkan/RIFactoryVk.h"

#include <vector>

namespace Fox
{
struct DRIFramebuffer
{
    std::vector<VkFormat> Format;
    VkExtent2D            Extent;
    VkFramebuffer         Framebuffer;
    DRIFramebuffer(VkFormat format, VkExtent2D extent, VkFramebuffer framebuffer) : Format(format), Extent(extent), Framebuffer(framebuffer){};
    DRIFramebuffer(std::vector<VkFormat> formats, VkExtent2D extent, VkFramebuffer framebuffer) : Format(formats), Extent(extent), Framebuffer(framebuffer){};
};

const std::vector<VkPresentModeKHR> PresentationModes = {
    VkPresentModeKHR::VK_PRESENT_MODE_IMMEDIATE_KHR, /*Immediate*/
    VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR, /*Hybrid vsync*/
    VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR, /*vsync*/
    VkPresentModeKHR::VK_PRESENT_MODE_FIFO_RELAXED_KHR, /*like FIFO except it behaves like IMMEDIATE in the case that it had no image in queue for the last VBLANK swap (i.e. prefers tearing
                                                           rather than showing old image).*/
};
const std::vector<VkSurfaceFormatKHR> Formats = { VkSurfaceFormatKHR{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
    VkSurfaceFormatKHR{ VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
    VkSurfaceFormatKHR{ VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR } };

struct DSwapchainInfo
{
    /*Window Surface*/
    VkSurfaceKHR                    Surface{};
    std::vector<VkSurfaceFormatKHR> SurfaceFormatsSupported;
    std::vector<VkPresentModeKHR>   SurfacePresentModesSupported;
    VkExtent2D                      SurfaceExtent{ 0, 0 };

    /*Swapchain*/
    VkSwapchainKHR       Swapchain{};
    VkSurfaceFormatKHR   SwapchainFormat{ VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_MAX_ENUM_KHR };
    VkPresentModeKHR     SwapchainPresentMode{ VK_PRESENT_MODE_MAX_ENUM_KHR };
    std::vector<VkImage> SwapchainImages;
    /*Imageviews*/
    std::vector<VkImageView>    ImageViews;
    std::vector<DRIFramebuffer> Framebuffers;

    /*image index used to index the framebuffer*/
    uint32_t CurrentImageIndex{ 0 };
    /*Is is still valid to render to*/
    bool Outdated{ false };

    bool IsFormatSupported(const VkSurfaceFormatKHR& format) const
    {
        const auto it =
        std::find_if(SurfaceFormatsSupported.begin(), SurfaceFormatsSupported.end(), [format](const VkSurfaceFormatKHR& f) { return f.format == format.format && f.colorSpace == format.colorSpace; });
        return it != SurfaceFormatsSupported.end();
    };

    bool IsPresentModeSupported(const VkPresentModeKHR& mode) const
    {
        const auto it = std::find(SurfacePresentModesSupported.begin(), SurfacePresentModesSupported.end(), mode);
        return it != SurfacePresentModesSupported.end();
    };
};
}