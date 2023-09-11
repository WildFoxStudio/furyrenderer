// Copyright RedFox Studio 2022

#pragma once

#include "VulkanDevice3.h"

#include <volk.h>

#include <functional>
#include <unordered_set>

namespace Fox
{

struct RIVulkanBuffer
{
    VkBuffer      Buffer{};
    VmaAllocation Allocation{};
};

class RIVulkanBufferHasher
{
  public:
    size_t operator()(RIVulkanBuffer const& key) const
    {
        std::hash<VkBuffer>      bh;
        std::hash<VmaAllocation> vh;
        std::hash<size_t>        sh;
        // hash each pointer then hash the sum of the two previous
        const size_t hash = sh(bh(key.Buffer) + vh(key.Allocation));
        return hash;
    }
};

class RIVulkanBufferEqualFn
{
  public:
    bool operator()(RIVulkanBuffer const& t1, RIVulkanBuffer const& t2) const { return t1.Buffer == t2.Buffer && t1.Allocation == t2.Allocation; }
};

// buffers implementation
class RIVulkanDevice4 : public RIVulkanDevice3
{

  public:
    virtual ~RIVulkanDevice4();

    RIVulkanBuffer CreateBufferHostVisible(uint32_t size, VkBufferUsageFlags usage);
    RIVulkanBuffer CreateBufferDeviceLocalTransferBit(uint32_t size, VkBufferUsageFlags usage);
    void           DestroyBuffer(const RIVulkanBuffer& buffer);
    void*          MapBuffer(const RIVulkanBuffer& buffer);
    void           UnmapBuffer(const RIVulkanBuffer& buffer);

  private:
    std::unordered_set<RIVulkanBuffer, RIVulkanBufferHasher, RIVulkanBufferEqualFn> _buffers;
};
}