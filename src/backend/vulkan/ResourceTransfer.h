// Copyright RedFox Studio 2022

#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

// #include "Image/ImageLoader.h"
// #include "RenderInterface/vulkan/RIFactoryVk.h"

namespace Fox
{
class CResourceTransfer
{
  public:
    CResourceTransfer(VkCommandBuffer command);
    // void CopyImage(VkBuffer sourceBuffer, VkImage destination, VkExtent2D extent, const DImage& image, size_t beginOffset = 0);
    void CopyMipMap(VkBuffer sourceBuffer, VkImage destination, VkExtent2D extent, uint32_t mipIndex, uint32_t internalOffset, size_t sourceOffset);
    void CopyBuffer(VkBuffer sourceBuffer, VkBuffer destination, size_t length, size_t beginOffset = 0);
    void BlitMipMap_DEPRECATED(VkImage src, VkImage destination, VkExtent2D extent, uint32_t mipIndex);
    void CopyImageToBuffer(VkBuffer destBuffer, VkImage sourceImage, VkExtent2D extent, uint32_t mipIndex);
    void FinishCommandBuffer();

  private:
    VkCommandBuffer _command{};
};

}