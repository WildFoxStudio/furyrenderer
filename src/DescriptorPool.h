// Copyright RedFox Studio 2022

#pragma once

#include "Core/renderInterface/RenderInterface.h"

#include <vector>

#define VK_USE_PLATFORM_WIN32_KHR
#include <volk.h>

namespace Fox
{

struct DRIDescriptorSetPool
{
    DRIDescriptorSetPool(ARIFactory* factory, const std::vector<RIShaderDescriptorBindings>& setBindings, VkDescriptorSetLayout layout);
    ~DRIDescriptorSetPool();

    uint32_t              Allocated() const { return (uint32_t)_used.size(); };
    VkDescriptorSet       Allocate();
    void                  ResetPool();
    VkDescriptorSetLayout GetDescriptorSetLayout() const { return _descriptorSetLayout; };

  private:
    static constexpr uint32_t                      MAX_SET_PER_POOL = 100;
    ARIFactory*                                    _factory;
    VkDescriptorSetLayout                          _descriptorSetLayout{};
    std::unordered_map<VkDescriptorType, uint32_t> _typeToCountMap;
    std::list<VkDescriptorPool>                    _pools{};
    std::list<VkDescriptorSet>                     _used;
};

struct DRIDescriptorSetUpdater
{
    DRIDescriptorSetUpdater(VkDescriptorSet set) : _set(set){};

    void BindUniformBuffer(uint32_t bindingIndex, VkBuffer buffer, uint32_t offset, uint32_t bytes)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = buffer;
        bufferInfo.offset = (uint32_t)offset;
        bufferInfo.range  = bytes;

        _bufferInfo.push_back(std::move(bufferInfo));

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet           = _set;
        descriptorWrite.dstBinding       = bindingIndex;
        descriptorWrite.dstArrayElement  = 0;
        descriptorWrite.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptorWrite.descriptorCount  = 1;
        descriptorWrite.pImageInfo       = nullptr;
        descriptorWrite.pBufferInfo      = &_bufferInfo.back();
        descriptorWrite.pTexelBufferView = nullptr; // Optional

        _writeInfo.push_back(std::move(descriptorWrite));
    };

    void BindCombinedImageSamplerArray(uint32_t bindingIndex, const std::vector<std::pair<VkImageView, VkSampler>>& imageToSamplerArray)
    {
        check(imageToSamplerArray.size() > 0);

        _imageInfo.push_back({});

        for (auto& pair : imageToSamplerArray)
            {
                VkDescriptorImageInfo imageInfo{};
                imageInfo.imageView   = pair.first;
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo.sampler     = pair.second;

                _imageInfo.back().push_back(std::move(imageInfo));
            }

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet           = _set;
        descriptorWrite.dstBinding       = bindingIndex;
        descriptorWrite.dstArrayElement  = 0;
        descriptorWrite.descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount  = (uint32_t)_imageInfo.back().size();
        descriptorWrite.pImageInfo       = _imageInfo.back().data();
        descriptorWrite.pBufferInfo      = nullptr;
        descriptorWrite.pTexelBufferView = nullptr; // Optional

        _writeInfo.push_back(std::move(descriptorWrite));
    };

    inline const std::vector<VkWriteDescriptorSet>& GetWriteInfo() const { return _writeInfo; };

  private:
    VkDescriptorSet                               _set;
    std::list<VkDescriptorBufferInfo>             _bufferInfo;
    std::list<std::vector<VkDescriptorImageInfo>> _imageInfo;
    std::vector<VkWriteDescriptorSet>             _writeInfo;
};
}