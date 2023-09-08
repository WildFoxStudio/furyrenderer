// Copyright RedFox Studio 2022

#include "VulkanDevice11.h"

#include "Core/utils/debug.h"
#include "UtilsVK.h"

namespace Fox
{

RIVulkanDevice11::~RIVulkanDevice11() { check(_descriptorPools.size() == 0); }

std::vector<VkDescriptorSet>
RIDescriptorPoolManager::AllocateDescriptorSets(VkDescriptorSetLayout descriptorSetLayout, int num)
{

    std::vector<VkDescriptorSetLayout> pLayouts(num, descriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = _pool;
    allocInfo.descriptorSetCount = (uint32_t)num;
    allocInfo.pSetLayouts        = pLayouts.data(); // once created it cannot change setLayout

    std::vector<VkDescriptorSet> descriptorSet(num, nullptr);
    const VkResult   result = vkAllocateDescriptorSets(_device, &allocInfo, descriptorSet.data());
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkErrorString(result));
        }
    return descriptorSet;
}

RIDescriptorSet*
RIDescriptorPoolManager::CreateDescriptorSet(VkDescriptorSetLayout descriptorSetLayout)
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = _pool;
    allocInfo.descriptorSetCount = (uint32_t)1;
    allocInfo.pSetLayouts        = &descriptorSetLayout; // once created it cannot change setLayout

    if (_descriptorSets.size() < _maxSets)
        {

            VkDescriptorSet descriptorSet{};
            const VkResult  result = vkAllocateDescriptorSets(_device, &allocInfo, &descriptorSet);
            if (VKFAILED(result))
                {
                    throw std::runtime_error(VkErrorString(result));
                }

            _descriptorSets.emplace_back();
            _descriptorSets.back().DescriptorSet = descriptorSet;

            return &_descriptorSets.back();
        }

    // Get unused descriptorSet
    auto* recycled = _getFirstUnusedDescriptorSet();
    if (recycled != nullptr)
        {
            vkFreeDescriptorSets(_device, _pool, (uint32_t)1, &recycled->DescriptorSet);

            VkDescriptorSet descriptorSet{};
            const VkResult  result = vkAllocateDescriptorSets(_device, &allocInfo, &recycled->DescriptorSet);
            if (VKFAILED(result))
                {
                    throw std::runtime_error(VkErrorString(result));
                }

            return recycled;
        }

    check(0);
    throw std::runtime_error("No descriptor sets left");
}

void
RIDescriptorPoolManager::ResetDescriptorSet(const std::vector<RIDescriptorSet>& descriptorSets)
{
    std::vector<VkDescriptorSet> vulkanDescriptorSets(descriptorSets.size());

    std::transform(descriptorSets.cbegin(), descriptorSets.cend(), std::back_inserter(vulkanDescriptorSets), [](const RIDescriptorSet& s) {
        check(s.Count() == 0); // Must not be bound to a executing command buffer
        return s.DescriptorSet;
    });

    vkFreeDescriptorSets(_device, _pool, (uint32_t)vulkanDescriptorSets.size(), vulkanDescriptorSets.data());
}

RIDescriptorSetBinder
RIDescriptorPoolManager::CreateDescriptorSetBinder()
{
    return RIDescriptorSetBinder(_device, this, _cachedDescriptorSets);
}

void
RIDescriptorPoolManager::ResetPool()
{
    vkResetDescriptorPool(_device, _pool, NULL);

    _descriptorSets.clear();
    _cachedDescriptorSets.clear();
}

void
RIDescriptorPoolManager::UpdateDescriptorSet(std::vector<VkWriteDescriptorSet> write)
{
    vkUpdateDescriptorSets(_device, (uint32_t)write.size(), write.data(), 0, nullptr);
}

RIDescriptorSet*
RIDescriptorPoolManager::_getFirstUnusedDescriptorSet()
{
    for (RIDescriptorSet& e : _descriptorSets)
        {
            if (e.Count() == 0)
                {
                    return &e;
                }
        }
    return {};
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
    check(_currentUpdate.has_value());
    RIDescriptorSet* set = QueryOrMakeDescriptorSet(descriptorSetLayout);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, setIndex, 1, &set->DescriptorSet, (uint32_t)_dynamicOffsets.size(), _dynamicOffsets.data());

    set->IncreaseCounter();

    _currentUpdate = std::nullopt;
}

RIDescriptorSet*
RIDescriptorSetBinder::QueryOrMakeDescriptorSet(VkDescriptorSetLayout descriptorSetLayout)
{
    check(_currentUpdate.has_value()); // You must bind something first
    RIDescriptorSet* set = _queryDescriptorSet(descriptorSetLayout, _currentUpdate.value());

    if (!set)
        {
            set = _pool->CreateDescriptorSet(descriptorSetLayout);
            _currentUpdate.value().SetDstSet(set->DescriptorSet);
            vkUpdateDescriptorSets(_device, (uint32_t)_currentUpdate.value().WriteDescriptorSet.size(), _currentUpdate.value().WriteDescriptorSet.data(), 0, nullptr);

            _cachedDescriptorSets[descriptorSetLayout][_currentUpdate.value()] = set;
        }

    check(set != nullptr);

    return set;
}

RIDescriptorSet*
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
            throw std::runtime_error(VkErrorString(result));
        }

    _descriptorPools.insert(descriptorPool);

    return descriptorPool;
}

void
RIVulkanDevice11::DestroyDescriptorPool(VkDescriptorPool pool)
{
    vkDestroyDescriptorPool(Device, pool, nullptr);
    check(_descriptorPools.size() > 0);
    const auto found = _descriptorPools.find(pool);
    check(found != _descriptorPools.end());
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
            throw std::runtime_error(VkErrorString(result));
        }
    return descriptorSet;
}

RIDescriptorPoolManager*
RIVulkanDevice11::CreateDescriptorPool2(const std::vector<VkDescriptorPoolSize>& poolDimensions, uint32_t maxSets)
{
    check(0);
    VkDescriptorPool         descriptorPool = CreateDescriptorPool(poolDimensions, maxSets);
    RIDescriptorPoolManager* pool           = new RIDescriptorPoolManager(Device, descriptorPool, maxSets);
    return pool;
}

void
RIVulkanDevice11::ResetDescriptorSet(VkDescriptorPool pool, const std::vector<VkDescriptorSet>& descriptorSets)
{
    vkFreeDescriptorSets(Device, pool, (uint32_t)descriptorSets.size(), descriptorSets.data());
}

}