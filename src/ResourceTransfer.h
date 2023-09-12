// Copyright RedFox Studio 2022

#pragma once

#include "Image/ImageLoader.h"
#include "RenderInterface/vulkan/RIFactoryVk.h"

namespace Fox
{
class CResourceTransfer
{
  public:
    CResourceTransfer(VkCommandBuffer command);
    void CopyImage(VkBuffer sourceBuffer, VkImage destination, VkExtent2D extent, const DImage& image, size_t beginOffset = 0);
    void CopyMipMap(VkBuffer sourceBuffer, VkImage destination, VkExtent2D extent, uint32_t mipIndex, uint32_t internalOffset, size_t sourceOffset);
    void CopyVertexBuffer(VkBuffer sourceBuffer, VkBuffer destination, size_t length, size_t beginOffset = 0);
    void FinishCommandBuffer();

  private:
    VkCommandBuffer _command{};
};

}