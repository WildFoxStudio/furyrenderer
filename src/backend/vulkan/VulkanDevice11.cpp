// Copyright RedFox Studio 2022

#include "VulkanDevice11.h"

#include "asserts.h"

#include "UtilsVK.h"

namespace Fox
{

RIVulkanDevice11::~RIVulkanDevice11() { furyassert(_descriptorPools.size() == 0); }

VkDescriptorSet
RIDescriptorPoolManager::CreateDescriptorSet(VkDescriptorSetLayout descriptorSetLayout)
{
    return DRIDescriptorSetAllocator::Allocate(descriptorSetLayout);
}

RIDescriptorSetBinder
RIDescriptorPoolManager::CreateDescriptorSetBinder()
{
    return RIDescriptorSetBinder(_device, this, _cachedDescriptorSets);
}

void
RIDescriptorPoolManager::ResetPool()
{
    DRIDescriptorSetAllocator::ResetPool();
    _cachedDescriptorSets.clear();
}

void
RIDescriptorPoolManager::UpdateDescriptorSet(std::vector<VkWriteDescriptorSet> write)
{
    vkUpdateDescriptorSets(_device, (uint32_t)write.size(), write.data(), 0, nullptr);
}

void
RIDescriptorSetBinder::BindUniformBuffer(uint32_t bindingIndex, VkBuffer buffer, uint32_t offset, uint32_t bytes)
{
    if (!_currentUpdate.has_value())
        _currentUpdate = RIDescriptorSetWrite{};

    _currentUpdate.value().BufferInfo.push_back({});

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = (uint32_t)offset;
    bufferInfo.range  = bytes;

    _currentUpdate.value().BufferInfo.back().push_back(std::move(bufferInfo));

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet           = nullptr; // not needed for hashing lookup
    descriptorWrite.dstBinding       = bindingIndex;
    descriptorWrite.dstArrayElement  = 0;
    descriptorWrite.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount  = 1;
    descriptorWrite.pImageInfo       = nullptr;
    descriptorWrite.pBufferInfo      = _currentUpdate.value().BufferInfo.back().data();
    descriptorWrite.pTexelBufferView = nullptr; // Optional

    _currentUpdate.value().WriteDescriptorSet.push_back(std::move(descriptorWrite));
}

void
RIDescriptorSetBinder::BindUniformBufferDynamic(uint32_t bindingIndex, VkBuffer buffer, uint32_t offset, uint32_t bytes, uint32_t dynamicOffset)
{
    if (!_currentUpdate.has_value())
        _currentUpdate = RIDescriptorSetWrite{};

    _currentUpdate.value().BufferInfo.push_back({});

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = (uint32_t)offset;
    bufferInfo.range  = bytes;

    _currentUpdate.value().BufferInfo.back().push_back(std::move(bufferInfo));

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet           = nullptr; // not needed for hashing lookup
    descriptorWrite.dstBinding       = bindingIndex;
    descriptorWrite.dstArrayElement  = 0;
    descriptorWrite.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descriptorWrite.descriptorCount  = 1;
    descriptorWrite.pImageInfo       = nullptr;
    descriptorWrite.pBufferInfo      = _currentUpdate.value().BufferInfo.back().data();
    descriptorWrite.pTexelBufferView = nullptr; // Optional

    _currentUpdate.value().WriteDescriptorSet.push_back(std::move(descriptorWrite));

    _dynamicOffsets.push_back(dynamicOffset);
}

void
RIDescriptorSetBinder::BindCombinedImageSamplerArray(uint32_t bindingIndex, const std::vector<std::pair<VkImageView, VkSampler>>& imageToSamplerArray)
{
    if (!_currentUpdate.has_value())
        _currentUpdate = RIDescriptorSetWrite{};

    _currentUpdate.value().ImageInfo.push_back({});

    for (auto& pair : imageToSamplerArray)
        {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageView   = pair.first;
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.sampler     = pair.second;

            _currentUpdate.value().ImageInfo.back().push_back(std::move(imageInfo));
        }

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet           = nullptr; // not needed for hashing lookup
    descriptorWrite.dstBinding       = bindingIndex;
    descriptorWrite.dstArrayElement  = 0;
    descriptorWrite.descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount  = (uint32_t)_currentUpdate.value().ImageInfo.back().size();
    descriptorWrite.pImageInfo       = _currentUpdate.value().ImageInfo.back().data();
    descriptorWrite.pBufferInfo      = nullptr;
    descriptorWrite.pTexelBufferView = nullptr; // Optional

    _currentUpdate.value().WriteDescriptorSet.push_back(std::move(descriptorWrite));
}

void
RIDescriptorSetBinder::BindDescriptorSet(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, VkDescriptorSetLayout descriptorSetLayout, uint32_t setIndex)
{
    furyassert(_currentUpdate.has_value());
    VkDescriptorSet set = QueryOrMakeDescriptorSet(descriptorSetLayout);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, setIndex, 1, &set, (uint32_t)_dynamicOffsets.size(), _dynamicOffsets.data());

    _currentUpdate = std::nullopt;
}

VkDescriptorSet
RIDescriptorSetBinder::QueryOrMakeDescriptorSet(VkDescriptorSetLayout descriptorSetLayout)
{
    furyassert(_currentUpdate.has_value()); // You must bind something first
    VkDescriptorSet set = _queryDescriptorSet(descriptorSetLayout, _currentUpdate.value());

    if (!set)
        {
            set = _poolManager->CreateDescriptorSet(descriptorSetLayout);
            _currentUpdate.value().SetDstSet(set);
            vkUpdateDescriptorSets(_device, (uint32_t)_currentUpdate.value().WriteDescriptorSet.size(), _currentUpdate.value().WriteDescriptorSet.data(), 0, nullptr);

            _cachedDescriptorSets[descriptorSetLayout][_currentUpdate.value()] = set;
        }

    furyassert(set != nullptr);

    return set;
}

VkDescriptorSet
RIDescriptorSetBinder::_queryDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, const RIDescriptorSetWrite& write)
{
    auto existing = _cachedDescriptorSets.find(descriptorSetLayout);
    if (existing != _cachedDescriptorSets.end())
        {
            auto foundEqual = existing->second.find(_currentUpdate.value());
            if (foundEqual != existing->second.end())
                {
                    return foundEqual->second;
                }
        }
    return nullptr;
}

VkDescriptorPool
RIVulkanDevice11::CreateDescriptorPool(const std::vector<VkDescriptorPoolSize>& poolDimensions, uint32_t maxSets)
{
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.poolSizeCount = (uint32_t)poolDimensions.size(); // is the number of elements in pPoolSizes.
    poolInfo.pPoolSizes    = poolDimensions.data(); // is a pointer to an array of VkDescriptorPoolSize structures
    poolInfo.maxSets       = maxSets; //  is the maximum number of -descriptor sets- that can be allocated from the pool (must be grater than 0)

    VkDescriptorPool descriptorPool{};
    const VkResult   result = vkCreateDescriptorPool(Device, &poolInfo, nullptr, &descriptorPool);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkUtils::VkErrorString(result));
        }

    _descriptorPools.insert(descriptorPool);

    return descriptorPool;
}

void
RIVulkanDevice11::DestroyDescriptorPool(VkDescriptorPool pool)
{
    vkDestroyDescriptorPool(Device, pool, nullptr);
    furyassert(_descriptorPools.size() > 0);
    const auto found = _descriptorPools.find(pool);
    furyassert(found != _descriptorPools.end());
    _descriptorPools.erase(found);
}

VkDescriptorSet
RIVulkanDevice11::CreateDescriptorSet(VkDescriptorPool pool, VkDescriptorSetLayout descriptorSetLayout)
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = pool;
    allocInfo.descriptorSetCount = (uint32_t)1;
    allocInfo.pSetLayouts        = &descriptorSetLayout;

    VkDescriptorSet descriptorSet{};
    const VkResult  result = vkAllocateDescriptorSets(Device, &allocInfo, &descriptorSet);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkUtils::VkErrorString(result));
        }
    return descriptorSet;
}

RIDescriptorPoolManager*
RIVulkanDevice11::CreateDescriptorPool2(const std::vector<VkDescriptorPoolSize>& poolDimensions, uint32_t maxSets)
{
    furyassert(0);
    VkDescriptorPool         descriptorPool = CreateDescriptorPool(poolDimensions, maxSets);
    RIDescriptorPoolManager* pool           = new RIDescriptorPoolManager(Device, poolDimensions, maxSets);
    return pool;
}

void
RIVulkanDevice11::ResetDescriptorSet_DEPRECATED(VkDescriptorPool pool, const std::vector<VkDescriptorSet>& descriptorSets)
{
    vkFreeDescriptorSets(Device, pool, (uint32_t)descriptorSets.size(), descriptorSets.data());
}

}