// Copyright RedFox Studio 2022

#include "DescriptorPool.h"
#include "RIFactoryVk.h"
#include "RenderInterface/NullThread.h"

namespace Fox
{
DRIDescriptorSetPool::DRIDescriptorSetPool(ARIFactory* factory, const std::vector<RIShaderDescriptorBindings>& setBindings, VkDescriptorSetLayout layout)
  : _factory(factory), _descriptorSetLayout(layout)
{
    for (const auto& binding : setBindings)
        {
            VkDescriptorType type;

            switch (binding.StorageType)
                {
                    case RIShaderDescriptorBindings::Type::STATIC:
                        type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        break;
                    case RIShaderDescriptorBindings::Type::DYNAMIC:
                        type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                        break;
                    case RIShaderDescriptorBindings::Type::SAMPLER_COMBINED:
                        type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        break;
                    default:
                        critical(0);
                        break;
                }
            auto existingType = _typeToCountMap.find(type);
            if (existingType != _typeToCountMap.end())
                {
                    existingType->second += binding.Count;
                }
            else
                _typeToCountMap[type] = binding.Count;
        }

    RIFactoryVk* ffactory = (RIFactoryVk*)_factory;
    _pools.push_back(ffactory->MT_CreateDescriptorPool(_typeToCountMap, MAX_SET_PER_POOL));
}

DRIDescriptorSetPool::~DRIDescriptorSetPool()
{
    RIFactoryVk* factory = (RIFactoryVk*)_factory;
    for (auto pool : _pools)
        {
            factory->MT_DestroyDescriptorPool(pool);
        }

    factory->MT_DestroyDescriptorSetLayout(_descriptorSetLayout);
}

VkDescriptorSet
DRIDescriptorSetPool::Allocate()
{
    RIFactoryVk* factory = (RIFactoryVk*)_factory;

    const uint32_t poolIndex = (uint32_t)std::floor(_used.size() / (double)MAX_SET_PER_POOL) + 1;
    if (poolIndex > _pools.size())
        {
            _pools.push_back(factory->MT_CreateDescriptorPool(_typeToCountMap, MAX_SET_PER_POOL));
        }

    _used.push_back(factory->MT_CreateDescriptorSet(_pools.back(), { _descriptorSetLayout }).back());
    return _used.back();
}

void
DRIDescriptorSetPool::ResetPool()
{
    RIFactoryVk* factory = (RIFactoryVk*)CNullThread::GetInstance()->GetContext()->QueryFactoryInterface();

    for (auto pool : _pools)
        {
            factory->MT_DestroyDescriptorPool(pool);
        }
    _pools.clear();
    _used.clear();
}

}