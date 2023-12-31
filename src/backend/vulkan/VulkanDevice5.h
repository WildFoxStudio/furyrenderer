// Copyright RedFox Studio 2022

#pragma once

#include "VulkanDevice4.h"

#include <volk.h>

#include <functional>
#include <unordered_set>

namespace Fox
{

struct RIVulkanImage
{
    VkImage           Image{};
    VmaAllocation     Allocation{};
    VkFormat          Format{};
    uint32_t          Width{};
    uint32_t          Height{};
    uint32_t          MipLevels{};
    VkImageUsageFlags UsageFlags{};
};

class RIVulkanImageHasher
{
  public:
    size_t operator()(RIVulkanImage const& key) const
    {
        std::hash<VkImage>       bh;
        std::hash<VmaAllocation> vh;
        std::hash<size_t>        sh;
        // hash each pointer then hash the sum of the two previous
        const size_t hash = sh(bh(key.Image) + vh(key.Allocation));
        return hash;
    }
};

class RIVulkanImageEqualFn
{
  public:
    bool operator()(RIVulkanImage const& t1, RIVulkanImage const& t2) const { return t1.Image == t2.Image && t1.Allocation == t2.Allocation; }
};

// images implementation
class RIVulkanDevice5 : public RIVulkanDevice4
{

  public:
    virtual ~RIVulkanDevice5();

    RIVulkanImage CreateImageDeviceLocal(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageUsageFlags usage, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED);
    RIVulkanImage CreateImageHostVisible(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageUsageFlags usage, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL);
    void          DestroyImage(const RIVulkanImage& image);
    VkImageView   CreateImageView_DEPRECATED(VkFormat format, const RIVulkanImage& image, VkImageAspectFlags aspect, uint32_t baseMipLevel, uint32_t mipmapCount);
    VkResult      CreateImageView(VkFormat format, VkImage image, VkImageAspectFlags aspect, uint32_t baseMipLevel, uint32_t mipmapCount, VkImageView* outImageView);
    void          DestroyImageView(VkImageView imageView);
    void*         MapImage(const RIVulkanImage& image);
    void          UnmapImage(const RIVulkanImage& image);

  private:
    std::unordered_set<RIVulkanImage, RIVulkanImageHasher, RIVulkanImageEqualFn> _images;
    std::unordered_set<VkImageView>                                              _imageViews;
};
}