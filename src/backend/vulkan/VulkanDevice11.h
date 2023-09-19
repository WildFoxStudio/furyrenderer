// Copyright RedFox Studio 2022

#pragma once

#include "RIResource.h"
#include "UtilsVK.h"
#include "VulkanDevice10.h"

#include <volk.h>

#include <algorithm>
#include <list>
#include <optional>
#include <unordered_set>

namespace Fox
{

/**
 * \brief This class hold N descriptor set pools where the dimensions correspond to an array of descriptor set layouts used to create a particular pipeline layout
 * One pool per frame, assume that all descriptor sets are used in a single frame, when frame finished we can free all the descriptor sets and destroy pools after the first one
 */
struct DRIDescriptorSetAllocator
{
    DRIDescriptorSetAllocator(VkDevice& device, const std::vector<VkDescriptorPoolSize>& poolDimensions, const uint32_t maxSetsPerPool)
      : _device(device), _poolDimensions(poolDimensions), _maxSetPerPool(maxSetsPerPool)
    {
    }
    ~DRIDescriptorSetAllocator()
    {
        for (auto pool : _pools)
            {
                vkDestroyDescriptorPool(_device, pool, nullptr);
            }
    }

    VkDescriptorSet Allocate(VkDescriptorSetLayout layout)
    {
        const uint32_t poolIndex = (uint32_t)std::floor(_countAllocated / (double)_maxSetPerPool) + 1;
        if (poolIndex > _pools.size())
            {
                VkDescriptorPoolCreateInfo poolInfo{};
                poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
                poolInfo.poolSizeCount = (uint32_t)_poolDimensions.size(); // is the number of elements in pPoolSizes.
                poolInfo.pPoolSizes    = _poolDimensions.data(); // is a pointer to an array of VkDescriptorPoolSize structures
                poolInfo.maxSets       = _maxSetPerPool; //  is the maximum number of -descriptor sets- that can be allocated from the pool (must be grater than 0)

                VkDescriptorPool descriptorPool{};
                const VkResult   result = vkCreateDescriptorPool(_device, &poolInfo, nullptr, &descriptorPool);
                if (VKFAILED(result))
                    {
                        throw std::runtime_error(VkUtils::VkErrorString(result));
                    }
                _pools.push_back(descriptorPool);
            }

        _countAllocated++;

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = _pools[poolIndex];
        allocInfo.descriptorSetCount = (uint32_t)1;
        allocInfo.pSetLayouts        = &layout;

        VkDescriptorSet descriptorSet{};
        const VkResult  result = vkAllocateDescriptorSets(_device, &allocInfo, &descriptorSet);
        if (VKFAILED(result))
            {
                throw std::runtime_error(VkUtils::VkErrorString(result));
            }
        return descriptorSet;
    }

    /*All of the descriptor sets must not be in use*/
    void ResetPool()
    {
        for (auto pool : _pools)
            {
                vkDestroyDescriptorPool(_device, pool, nullptr);
            }

        _pools.clear();
        _countAllocated = 0;
    }

  private:
    VkDevice                                _device;
    const std::vector<VkDescriptorPoolSize> _poolDimensions;
    const uint32_t                          _maxSetPerPool;
    std::vector<VkDescriptorPool>           _pools{};
    uint32_t                                _countAllocated;
};

/**
 * \brief This struct is used to cache and also update a descriptor set
 * It contains all the info needed to update the descriptor set
 */
struct RIDescriptorSetWrite
{
    std::vector<VkWriteDescriptorSet>              WriteDescriptorSet;
    std::list<std::vector<VkDescriptorBufferInfo>> BufferInfo;
    std::list<std::vector<VkDescriptorImageInfo>>  ImageInfo;

    void SetDstSet(VkDescriptorSet ptr)
    {
        for (auto& e : WriteDescriptorSet)
            {
                e.dstSet = ptr;
            }
    };
};

struct HashVkDescriptorBufferInfo
{
    size_t operator()(const VkDescriptorBufferInfo& bufferInfo) const
    {
        size_t hashValue = 0;

        // Combine hashes of the members
        hashValue ^= std::hash<VkBuffer>()(bufferInfo.buffer);
        hashValue ^= std::hash<VkDeviceSize>()(bufferInfo.offset);
        hashValue ^= std::hash<VkDeviceSize>()(bufferInfo.range);

        return hashValue;
    }
};

struct HashVkDescriptorImageInfo
{
    size_t operator()(const VkDescriptorImageInfo& imageInfo) const
    {
        size_t hashValue = 0;

        // Combine hashes of the members
        hashValue ^= std::hash<VkSampler>()(imageInfo.sampler);
        hashValue ^= std::hash<VkImageView>()(imageInfo.imageView);
        hashValue ^= std::hash<VkImageLayout>()(imageInfo.imageLayout);

        return hashValue;
    }
};

struct HashVkWriteDescriptorSet
{
    size_t operator()(const VkWriteDescriptorSet& writeDescriptorSet) const
    {
        size_t hashValue = 0;

        // Combine hashes of the members
        hashValue ^= std::hash<VkStructureType>()(writeDescriptorSet.sType);
        // hashValue ^= std::hash<const void*>()(writeDescriptorSet.pNext);
        //  hashValue ^= std::hash<VkDescriptorSet>()(writeDescriptorSet.dstSet);//We don't need to consider the dstSet
        hashValue ^= std::hash<uint32_t>()(writeDescriptorSet.dstBinding);
        hashValue ^= std::hash<uint32_t>()(writeDescriptorSet.dstArrayElement);
        hashValue ^= std::hash<VkDescriptorType>()(writeDescriptorSet.descriptorType);
        hashValue ^= std::hash<uint32_t>()(writeDescriptorSet.descriptorCount);

        if (writeDescriptorSet.pImageInfo != nullptr)
            {
                size_t imageInfoHash = 0;
                for (uint32_t i = 0; i < writeDescriptorSet.descriptorCount; i++)
                    {
                        const VkDescriptorImageInfo* imageInfoPtr = writeDescriptorSet.pImageInfo + i;
                        HashVkDescriptorImageInfo    hashFn;
                        imageInfoHash ^= hashFn(*imageInfoPtr);
                    }
                hashValue ^= imageInfoHash;
            }
        if (writeDescriptorSet.pBufferInfo != nullptr)
            {
                size_t bufferInfoHash = 0;
                for (uint32_t i = 0; i < writeDescriptorSet.descriptorCount; i++)
                    {
                        const VkDescriptorBufferInfo* bufferInfoPtr = writeDescriptorSet.pBufferInfo + i;
                        HashVkDescriptorBufferInfo    hashFn;
                        bufferInfoHash ^= hashFn(*bufferInfoPtr);
                    }
                hashValue ^= bufferInfoHash;
            }

        // hashValue ^= std::hash<const VkDescriptorImageInfo*>()(writeDescriptorSet.pImageInfo);// don't hash pointers
        // hashValue ^= std::hash<const VkDescriptorBufferInfo*>()(writeDescriptorSet.pBufferInfo);// don't hash pointers

        return hashValue;
    }
};

struct RIDescriptorSetWriteHashFn
{
    size_t operator()(const RIDescriptorSetWrite& lhs) const
    {
        size_t hash{};

        for (const auto& e : lhs.WriteDescriptorSet)
            {
                HashVkWriteDescriptorSet hashFn;
                hash ^= hashFn(e);
            }

        return hash;
    }
};

struct RIDescriptorSetWriteEqualFn
{
    bool operator()(const RIDescriptorSetWrite& lhs, const RIDescriptorSetWrite& rhs) const
    {
        if (lhs.WriteDescriptorSet.size() == rhs.WriteDescriptorSet.size() && lhs.BufferInfo.size() == rhs.BufferInfo.size() && lhs.ImageInfo.size() == rhs.ImageInfo.size())
            {
                const bool equal1 =
                std::equal(lhs.WriteDescriptorSet.begin(), lhs.WriteDescriptorSet.end(), rhs.WriteDescriptorSet.begin(), rhs.WriteDescriptorSet.end(), [&](const auto& a, const auto& b) {
                    return CompareVkWriteDescriptorSet(a, b); // Customize the comparison logic here
                });
                if (!equal1)
                    return equal1;
                const bool equal2 = std::equal(lhs.BufferInfo.begin(), lhs.BufferInfo.end(), rhs.BufferInfo.begin(), rhs.BufferInfo.end(), [&](const auto& a, const auto& b) {
                    return std::equal(a.begin(), a.end(), b.begin(), b.end(), [&](const auto& aa, const auto& bb) {
                        return CompareVkDescriptorBufferInfo(aa, bb); // Customize the comparison logic here
                    });
                });
                if (!equal2)
                    return equal2;
                const bool equal3 = std::equal(lhs.ImageInfo.begin(), lhs.ImageInfo.end(), rhs.ImageInfo.begin(), rhs.ImageInfo.end(), [&](const auto& a, const auto& b) {
                    return std::equal(a.begin(), a.end(), b.begin(), b.end(), [&](const auto& aa, const auto& bb) {
                        return CompareVkDescriptorImageInfo(aa, bb); // Customize the comparison logic here
                    });
                });
                if (!equal3)
                    return equal3;

                return true;
            }
        return false;
    }
    bool CompareVkWriteDescriptorSet(const VkWriteDescriptorSet& left, const VkWriteDescriptorSet& right) const
    {
        return left.sType == right.sType && left.pNext == right.pNext && /*Do not compare dstSet*/ left.dstBinding == right.dstBinding && left.dstArrayElement == right.dstArrayElement &&
        left.descriptorCount == right.descriptorCount && left.descriptorType == right.descriptorType; // do not compare pointers
    }
    bool CompareVkDescriptorBufferInfo(const VkDescriptorBufferInfo& left, const VkDescriptorBufferInfo& right) const
    {
        return left.buffer == right.buffer && left.offset == right.offset && left.range == right.range;
    }
    bool CompareVkDescriptorImageInfo(const VkDescriptorImageInfo& left, const VkDescriptorImageInfo& right) const
    {
        return left.sampler == right.sampler && left.imageView == right.imageView && left.imageLayout == right.imageLayout;
    }
};

struct RIDescriptorPoolManagerInterface
{
    RIDescriptorPoolManagerInterface() = default;

    virtual VkDescriptorSet CreateDescriptorSet(VkDescriptorSetLayout descriptorSetLayout) = 0;
};

/**
 * \brief This class is responsible to cache and update descriptor sets. Based on the bounded resources it can return an existing identical descriptor set or
 * it allocated and updates a new one for you. The interface is simplified for binding resources.
 */
struct RIDescriptorSetBinder
{
    using DescriptorSetCacheMap                      = std::unordered_map<RIDescriptorSetWrite, VkDescriptorSet, RIDescriptorSetWriteHashFn, RIDescriptorSetWriteEqualFn>;
    using DescriptorSetLayoutToDescriptorSetCacheMap = std::unordered_map<VkDescriptorSetLayout, DescriptorSetCacheMap>;
    RIDescriptorSetBinder(VkDevice device, RIDescriptorPoolManagerInterface* pool, DescriptorSetLayoutToDescriptorSetCacheMap& cachedDescriptorSets)
      : _device(device), _poolManager(pool), _cachedDescriptorSets(cachedDescriptorSets)
    {
    }

    void            BindUniformBuffer(uint32_t bindingIndex, VkBuffer buffer, uint32_t offset, uint32_t bytes);
    void            BindUniformBufferDynamic(uint32_t bindingIndex, VkBuffer buffer, uint32_t offset, uint32_t bytes, uint32_t dynamicOffset);
    void            BindCombinedImageSamplerArray(uint32_t bindingIndex, const std::vector<std::pair<VkImageView, VkSampler>>& imageToSamplerArray);
    void            BindDescriptorSet(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, VkDescriptorSetLayout descriptorSetLayout, uint32_t setIndex);
    VkDescriptorSet QueryOrMakeDescriptorSet(VkDescriptorSetLayout descriptorSetLayout);

  private:
    VkDevice                                    _device;
    RIDescriptorPoolManagerInterface*           _poolManager;
    DescriptorSetLayoutToDescriptorSetCacheMap& _cachedDescriptorSets;
    std::optional<RIDescriptorSetWrite>         _currentUpdate;
    std::vector<uint32_t>                       _dynamicOffsets;

    VkDescriptorSet _queryDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, const RIDescriptorSetWrite& write);
};

/**
 * \brief This class is just an implementation to be used by the RIDescriptorSetBinder to allocate new descriptor set. But is also store the cache to all the
 * allocated descriptor sets.
 */
struct RIDescriptorPoolManager
  : public RIDescriptorPoolManagerInterface
  , public DRIDescriptorSetAllocator
{
    RIDescriptorPoolManager(VkDevice device, const std::vector<VkDescriptorPoolSize>& poolDimensions, const uint32_t maxSetsPerPool)
      : _device(device), DRIDescriptorSetAllocator(device, poolDimensions, maxSetsPerPool){};

    VkDescriptorSet       CreateDescriptorSet(VkDescriptorSetLayout descriptorSetLayout) override;
    RIDescriptorSetBinder CreateDescriptorSetBinder();
    void                  ResetPool();
    void                  UpdateDescriptorSet(std::vector<VkWriteDescriptorSet> write);

  private:
    VkDevice _device;

    RIDescriptorSetBinder::DescriptorSetLayoutToDescriptorSetCacheMap _cachedDescriptorSets; // used only by the RIDescriptorSetBinder class
};

class RIVulkanDevice11 : public RIVulkanDevice10
{

  public:
    virtual ~RIVulkanDevice11();

    VkDescriptorPool CreateDescriptorPool(const std::vector<VkDescriptorPoolSize>& poolDimensions, uint32_t maxSets);
    void             DestroyDescriptorPool(VkDescriptorPool pool);

    RIDescriptorPoolManager* CreateDescriptorPool2(const std::vector<VkDescriptorPoolSize>& poolDimensions, uint32_t maxSets);

    VkDescriptorSet CreateDescriptorSet(VkDescriptorPool pool, VkDescriptorSetLayout descriptorSetLayout);
    void            ResetDescriptorSet_DEPRECATED(VkDescriptorPool pool, const std::vector<VkDescriptorSet>& descriptorSets);

    RIDescriptorSetBinder CreateDescriptorSetBinder(RIDescriptorPoolManager* pool);

  private:
    std::unordered_set<VkDescriptorPool> _descriptorPools;
};
}