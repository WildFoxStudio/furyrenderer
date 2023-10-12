// Copyright RedFox Studio 2022

#include "VulkanDevice4.h"

#include "asserts.h"

#include "UtilsVK.h"

namespace Fox
{

RIVulkanDevice4::~RIVulkanDevice4() { check(_buffers.size() == 0); }

RIVulkanBuffer
RIVulkanDevice4::CreateBufferHostVisible(uint32_t size, VkBufferUsageFlags usage)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size        = size;
    bufferInfo.usage       = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    allocInfo.flags                   = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    RIVulkanBuffer buf;
    buf.IsMappable        = true;

    const VkResult result = vmaCreateBuffer(VmaAllocator, &bufferInfo, &allocInfo, &buf.Buffer, &buf.Allocation, nullptr);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkUtils::VkErrorString(result));
        }

    _buffers.insert(buf);

    return buf;
}

RIVulkanBuffer
RIVulkanDevice4::CreateBufferDeviceLocalTransferBit(uint32_t size, VkBufferUsageFlags usage)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size        = size;
    bufferInfo.usage       = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    allocInfo.flags                   = VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

    RIVulkanBuffer buf;

    const VkResult result = vmaCreateBuffer(VmaAllocator, &bufferInfo, &allocInfo, &buf.Buffer, &buf.Allocation, nullptr);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkUtils::VkErrorString(result));
        }

    _buffers.insert(buf);

    return buf;
}

void
RIVulkanDevice4::DestroyBuffer(const RIVulkanBuffer& buffer)
{
    vmaDestroyBuffer(VmaAllocator, buffer.Buffer, buffer.Allocation);
    _buffers.erase(_buffers.find(buffer));
}

void*
RIVulkanDevice4::MapBuffer(const RIVulkanBuffer& buffer)
{
    void*          gpuVirtualAddress{};
    const VkResult result = vmaMapMemory(VmaAllocator, buffer.Allocation, &gpuVirtualAddress);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkUtils::VkErrorString(result));
        }
    return gpuVirtualAddress;
}

void
RIVulkanDevice4::UnmapBuffer(const RIVulkanBuffer& buffer)
{
    vmaUnmapMemory(VmaAllocator, buffer.Allocation);
}

}