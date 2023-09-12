// Copyright RedFox Studio 2022

#include "VulkanDevice5.h"

#include "asserts.h"

#include "UtilsVK.h"

namespace Fox
{

RIVulkanDevice5::~RIVulkanDevice5()
{
    check(_images.size() == 0);
    check(_imageViews.size() == 0);
}

RIVulkanImage
RIVulkanDevice5::CreateImageDeviceLocal(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageUsageFlags usage)
{

    VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = mipLevels;
    imageInfo.arrayLayers   = 1;
    imageInfo.format        = format;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage         = usage;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    RIVulkanImage  image{};
    const VkResult result = vmaCreateImage(VmaAllocator, &imageInfo, &allocInfo, &image.Image, &image.Allocation, nullptr);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkUtils::VkErrorString(result));
        }

    image.Format     = format;
    image.Width      = width;
    image.Height     = height;
    image.MipLevels  = mipLevels;
    image.UsageFlags = usage;

    _images.insert(image);

    return image;
}

RIVulkanImage
RIVulkanDevice5::CreateImageHostVisible(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageUsageFlags usage)
{

    VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = mipLevels;
    imageInfo.arrayLayers   = 1;
    imageInfo.format        = format;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage         = usage;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;

    RIVulkanImage  image{};
    const VkResult result = vmaCreateImage(VmaAllocator, &imageInfo, &allocInfo, &image.Image, &image.Allocation, nullptr);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkUtils::VkErrorString(result));
        }

    image.Format     = format;
    image.Width      = width;
    image.Height     = height;
    image.MipLevels  = mipLevels;
    image.UsageFlags = usage;

    _images.insert(image);

    return image;
}

void
RIVulkanDevice5::DestroyImage(const RIVulkanImage& image)
{
    vmaDestroyImage(VmaAllocator, image.Image, image.Allocation);
    _images.erase(_images.find(image));
}

VkImageView
RIVulkanDevice5::CreateImageView(VkFormat format, const RIVulkanImage& image, VkImageAspectFlags aspect, uint32_t baseMipLevel, uint32_t mipmapCount)
{
    VkImageView imageView{};
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image                           = image.Image;
        viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format                          = format;
        viewInfo.subresourceRange.aspectMask     = aspect;
        viewInfo.subresourceRange.baseMipLevel   = baseMipLevel;
        viewInfo.subresourceRange.levelCount     = mipmapCount;
        viewInfo.subresourceRange.baseArrayLayer = 0; // first element of array
        viewInfo.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS; // till last array element

        const VkResult result = vkCreateImageView(Device, &viewInfo, nullptr, &imageView);
        if (VKFAILED(result))
            {
                throw std::runtime_error(VkUtils::VkErrorString(result));
                return nullptr;
            }
    }
    check(imageView);

    _imageViews.insert(imageView);

    return imageView;
}

void
RIVulkanDevice5::DestroyImageView(VkImageView imageView)
{
    vkDestroyImageView(Device, imageView, nullptr);
    _imageViews.erase(_imageViews.find(imageView));
}

}