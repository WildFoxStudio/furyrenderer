// Copyright RedFox Studio 2022

#include "ResourceTransfer.h"
#include "asserts.h"
#include <volk.h>

namespace Fox
{
CResourceTransfer::CResourceTransfer(VkCommandBuffer command) : _command(command)
{
    //check(_command);
    //VkCommandBufferBeginInfo beginInfo{};
    //beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    //beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    //vkBeginCommandBuffer(_command, &beginInfo);
}

// void
// CResourceTransfer::CopyImage(VkBuffer sourceBuffer, VkImage destination, VkExtent2D extent, const DImage& image, size_t beginOffset)
//{
//     std::vector<VkBufferImageCopy>    regions;
//     std::vector<VkImageMemoryBarrier> barriersTransfer;
//     std::vector<VkImageMemoryBarrier> barriersSampler;
//
//     check(beginOffset < std::numeric_limits<uint32_t>::max());
//     uint32_t offset    = (uint32_t)beginOffset;
//     uint32_t mipWidth  = image.Width;
//     uint32_t mipHeight = image.Height;
//     for (unsigned int mipMap = 0; mipMap < image.MipLeves; mipMap++)
//         {
//
//             VkBufferImageCopy region{};
//             {
//                 region.bufferOffset      = offset;
//                 region.bufferRowLength   = 0;
//                 region.bufferImageHeight = 0;
//
//                 region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
//                 region.imageSubresource.mipLevel       = mipMap;
//                 region.imageSubresource.baseArrayLayer = 0;
//                 region.imageSubresource.layerCount     = 1;
//
//                 region.imageOffset = { 0, 0, 0 };
//                 region.imageExtent = { mipWidth, mipHeight, 1 };
//             }
//
//             offset += (mipWidth * mipHeight) * image.BytesPerPixel;
//             regions.emplace_back(std::move(region));
//
//             mipWidth /= 2;
//             mipHeight /= 2;
//
//             VkImageMemoryBarrier barrierTransfer{};
//             {
//                 barrierTransfer.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
//                 barrierTransfer.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
//                 barrierTransfer.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
//                 barrierTransfer.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
//                 barrierTransfer.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
//                 barrierTransfer.image                           = destination;
//                 barrierTransfer.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
//                 barrierTransfer.subresourceRange.baseMipLevel   = mipMap;
//                 barrierTransfer.subresourceRange.levelCount     = 1; // number of mip maps
//                 barrierTransfer.subresourceRange.baseArrayLayer = 0;
//                 barrierTransfer.subresourceRange.layerCount     = 1;
//                 barrierTransfer.srcAccessMask                   = 0;
//                 barrierTransfer.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
//             }
//             barriersTransfer.emplace_back(std::move(barrierTransfer));
//
//             VkImageMemoryBarrier barrierSampler{};
//             {
//                 barrierSampler.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
//                 barrierSampler.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
//                 barrierSampler.newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//                 barrierSampler.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
//                 barrierSampler.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
//                 barrierSampler.image                           = destination;
//                 barrierSampler.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
//                 barrierSampler.subresourceRange.baseMipLevel   = mipMap;
//                 barrierSampler.subresourceRange.levelCount     = 1;
//                 barrierSampler.subresourceRange.baseArrayLayer = 0;
//                 barrierSampler.subresourceRange.layerCount     = 1;
//                 barrierSampler.srcAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
//                 barrierSampler.dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
//             }
//             barriersSampler.emplace_back(std::move(barrierSampler));
//         }
//
//     vkCmdPipelineBarrier(_command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, (uint32_t)barriersTransfer.size(), barriersTransfer.data());
//     vkCmdCopyBufferToImage(_command, sourceBuffer, destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)regions.size(), regions.data());
//     vkCmdPipelineBarrier(_command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, (uint32_t)barriersSampler.size(), barriersSampler.data());
// }

void
CResourceTransfer::CopyMipMap(VkBuffer sourceBuffer, VkImage destination, VkExtent2D extent, uint32_t mipIndex, uint32_t internalOffset, size_t sourceOffset)
{
    VkBufferImageCopy region{};
    {
        region.bufferOffset      = sourceOffset;
        region.bufferRowLength   = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel       = mipIndex;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = 1;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { extent.width, extent.height, 1 };
    }

    VkImageMemoryBarrier barrierTransfer{};
    {
        barrierTransfer.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrierTransfer.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        barrierTransfer.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrierTransfer.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrierTransfer.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrierTransfer.image                           = destination;
        barrierTransfer.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrierTransfer.subresourceRange.baseMipLevel   = mipIndex;
        barrierTransfer.subresourceRange.levelCount     = 1; // number of mip maps
        barrierTransfer.subresourceRange.baseArrayLayer = 0;
        barrierTransfer.subresourceRange.layerCount     = 1;
        barrierTransfer.srcAccessMask                   = 0;
        barrierTransfer.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
    }

    VkImageMemoryBarrier barrierSampler{};
    {
        barrierSampler.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrierSampler.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrierSampler.newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrierSampler.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrierSampler.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrierSampler.image                           = destination;
        barrierSampler.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrierSampler.subresourceRange.baseMipLevel   = mipIndex;
        barrierSampler.subresourceRange.levelCount     = 1;
        barrierSampler.subresourceRange.baseArrayLayer = 0;
        barrierSampler.subresourceRange.layerCount     = 1;
        barrierSampler.srcAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrierSampler.dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
    }

    vkCmdPipelineBarrier(_command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, (uint32_t)1, &barrierTransfer);
    vkCmdCopyBufferToImage(_command, sourceBuffer, destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)1, &region);
    vkCmdPipelineBarrier(_command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, (uint32_t)1, &barrierSampler);
}

void
CResourceTransfer::BlitMipMap_DEPRECATED(VkImage src, VkImage destination, VkExtent2D extent, uint32_t mipIndex)
{

    VkImageMemoryBarrier destinationBarrierTransfer{};
    {
        destinationBarrierTransfer.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        destinationBarrierTransfer.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        destinationBarrierTransfer.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        destinationBarrierTransfer.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        destinationBarrierTransfer.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        destinationBarrierTransfer.image                           = destination;
        destinationBarrierTransfer.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        destinationBarrierTransfer.subresourceRange.baseMipLevel   = mipIndex;
        destinationBarrierTransfer.subresourceRange.levelCount     = 1; // number of mip maps
        destinationBarrierTransfer.subresourceRange.baseArrayLayer = 0;
        destinationBarrierTransfer.subresourceRange.layerCount     = 1;
        destinationBarrierTransfer.srcAccessMask                   = 0;
        destinationBarrierTransfer.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
    }

    // Define the region to blit (we will blit the whole swapchain image)
    VkOffset3D blitSize;
    blitSize.x = extent.width;
    blitSize.y = extent.height;
    blitSize.z = 1;
    VkImageBlit imageBlitRegion{};
    imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBlitRegion.srcSubresource.layerCount = 1;
    imageBlitRegion.srcOffsets[1]             = blitSize;
    imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBlitRegion.dstSubresource.layerCount = 1;
    imageBlitRegion.dstOffsets[1]             = blitSize;

    vkCmdPipelineBarrier(_command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, (uint32_t)1, &destinationBarrierTransfer);
    // 1:1 blit ratio
    vkCmdBlitImage(_command, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlitRegion, VK_FILTER_NEAREST);
}

void
CResourceTransfer::CopyImageToBuffer(VkBuffer destBuffer, VkImage sourceImage, VkExtent2D extent, uint32_t mipIndex)
{
    VkBufferImageCopy region{};
    {
        region.bufferOffset      = 0;
        region.bufferRowLength   = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel       = mipIndex;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = 1;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { extent.width, extent.height, 1 };
    }

    VkImageMemoryBarrier barrierTransfer{};
    {
        barrierTransfer.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrierTransfer.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        barrierTransfer.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrierTransfer.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrierTransfer.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrierTransfer.image                           = sourceImage;
        barrierTransfer.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrierTransfer.subresourceRange.baseMipLevel   = mipIndex;
        barrierTransfer.subresourceRange.levelCount     = 1; // number of mip maps
        barrierTransfer.subresourceRange.baseArrayLayer = 0;
        barrierTransfer.subresourceRange.layerCount     = 1;
        barrierTransfer.srcAccessMask                   = 0;
        barrierTransfer.dstAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;
    }

    vkCmdPipelineBarrier(_command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, (uint32_t)1, &barrierTransfer);
    vkCmdCopyImageToBuffer(_command, sourceImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destBuffer, 1, &region);

    VkImageMemoryBarrier barrierSampler{};
    {
        barrierSampler.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrierSampler.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrierSampler.newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrierSampler.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrierSampler.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrierSampler.image                           = sourceImage;
        barrierSampler.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrierSampler.subresourceRange.baseMipLevel   = mipIndex;
        barrierSampler.subresourceRange.levelCount     = 1;
        barrierSampler.subresourceRange.baseArrayLayer = 0;
        barrierSampler.subresourceRange.layerCount     = 1;
        barrierSampler.srcAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;
        barrierSampler.dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
    }
    vkCmdPipelineBarrier(_command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, (uint32_t)1, &barrierSampler);
}

void
CResourceTransfer::CopyBuffer(VkBuffer sourceBuffer, VkBuffer destination, size_t length, size_t beginOffset)
{
    VkBufferCopy copy{};
    copy.srcOffset = beginOffset;
    copy.dstOffset = 0; // Write always to the beginning
    copy.size      = length;

    VkBufferMemoryBarrier readBarrier{};
    readBarrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    readBarrier.pNext               = NULL;
    readBarrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
    readBarrier.dstAccessMask       = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    readBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    readBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    readBarrier.buffer              = destination;
    readBarrier.offset              = 0;
    readBarrier.size                = VK_WHOLE_SIZE;

    vkCmdCopyBuffer(_command, sourceBuffer, destination, 1, &copy);
    vkCmdPipelineBarrier(_command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &readBarrier, 0, nullptr);
}

void
CResourceTransfer::FinishCommandBuffer()
{
    vkEndCommandBuffer(_command);
}

}