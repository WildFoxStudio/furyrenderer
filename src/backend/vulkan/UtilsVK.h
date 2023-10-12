// Copyright RedFox Studio 2022

#pragma once

#include "IContext.h"
#include "asserts.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include <volk.h>

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

#define VKFAILED(result) ((result != VK_SUCCESS ? true : false))
#define VKSUCCEDED(result) ((result == VK_SUCCESS ? true : false))

namespace VkUtils
{

inline const char*
VkErrorString(enum VkResult result)
{
    switch (result)
        {
            case VK_SUCCESS:
                return "VK_SUCCESS";
                break;
            case VK_NOT_READY:
                return "VK_NOT_READY";
                break;
            case VK_TIMEOUT:
                return "VK_TIMEOUT";
                break;
            case VK_EVENT_SET:
                return "VK_EVENT_SET";
                break;
            case VK_EVENT_RESET:
                return "VK_EVENT_RESET";
                break;
            case VK_INCOMPLETE:
                return "VK_INCOMPLETE";
                break;
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                return "VK_ERROR_OUT_OF_HOST_MEMORY";
                break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
                break;
            case VK_ERROR_INITIALIZATION_FAILED:
                return "VK_ERROR_INITIALIZATION_FAILED";
                break;
            case VK_ERROR_DEVICE_LOST:
                return "VK_ERROR_DEVICE_LOST";
                break;
            case VK_ERROR_MEMORY_MAP_FAILED:
                return "VK_ERROR_MEMORY_MAP_FAILED";
                break;
            case VK_ERROR_LAYER_NOT_PRESENT:
                return "VK_ERROR_LAYER_NOT_PRESENT";
                break;
            case VK_ERROR_EXTENSION_NOT_PRESENT:
                return "VK_ERROR_EXTENSION_NOT_PRESENT";
                break;
            case VK_ERROR_FEATURE_NOT_PRESENT:
                return "VK_ERROR_FEATURE_NOT_PRESENT";
                break;
            case VK_ERROR_INCOMPATIBLE_DRIVER:
                return "VK_ERROR_INCOMPATIBLE_DRIVER";
                break;
            case VK_ERROR_TOO_MANY_OBJECTS:
                return "VK_ERROR_TOO_MANY_OBJECTS";
                break;
            case VK_ERROR_FORMAT_NOT_SUPPORTED:
                return "VK_ERROR_FORMAT_NOT_SUPPORTED";
                break;
            case VK_ERROR_FRAGMENTED_POOL:
                return "VK_ERROR_FRAGMENTED_POOL";
                break;
            case VK_ERROR_UNKNOWN:
                return "VK_ERROR_UNKNOWN";
                break;
            default:
                return "UNKNOWN VK ERROR";
                break;
        }
};

inline VkPresentModeKHR
convertPresentMode(const Fox::EPresentMode mode)
{

    switch (mode)
        {
            case Fox::EPresentMode::IMMEDIATE_KHR:
                return VkPresentModeKHR::VK_PRESENT_MODE_IMMEDIATE_KHR;
            case Fox::EPresentMode::MAILBOX:
                return VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR;
            case Fox::EPresentMode::FIFO:
                return VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
            case Fox::EPresentMode::FIFO_RELAXED:
                return VkPresentModeKHR::VK_PRESENT_MODE_FIFO_RELAXED_KHR;
        }
    check(0);
    return VkPresentModeKHR::VK_PRESENT_MODE_MAX_ENUM_KHR;
};

inline Fox::EFormat
convertVkFormat(const VkFormat format)
{
    switch (format)
        {
            case VK_FORMAT_R8_UNORM:
                return Fox::EFormat::R8_UNORM;
            case VK_FORMAT_R8G8B8_UNORM:
                return Fox::EFormat::R8G8B8_UNORM;
            case VK_FORMAT_R8G8B8A8_UNORM:
                return Fox::EFormat::R8G8B8A8_UNORM;
            case VK_FORMAT_B8G8R8_UNORM:
                return Fox::EFormat::B8G8R8_UNORM;
            case VK_FORMAT_B8G8R8A8_UNORM:
                return Fox::EFormat::B8G8R8A8_UNORM;
            case VK_FORMAT_D16_UNORM:
                return Fox::EFormat::DEPTH16_UNORM;
            case VK_FORMAT_D32_SFLOAT:
                return Fox::EFormat::DEPTH32_FLOAT;
            case VK_FORMAT_D16_UNORM_S8_UINT:
                return Fox::EFormat::DEPTH16_UNORM_STENCIL8_UINT;
            case VK_FORMAT_D24_UNORM_S8_UINT:
                return Fox::EFormat::DEPTH24_UNORM_STENCIL8_UINT;
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                return Fox::EFormat::DEPTH32_FLOAT_STENCIL8_UINT;
            case VK_FORMAT_R32_SFLOAT:
                return Fox::EFormat::R32_FLOAT;
            case VK_FORMAT_R32G32_SFLOAT:
                return Fox::EFormat::R32G32_FLOAT;
            case VK_FORMAT_R32G32B32_SFLOAT:
                return Fox::EFormat::R32G32B32_FLOAT;
            case VK_FORMAT_R32G32B32A32_SFLOAT:
                return Fox::EFormat::R32G32B32A32_FLOAT;
        }

    check(0);
    return Fox::EFormat::R8_UNORM;
};

inline std::string
vkFormatString(const VkFormat format)
{
    switch (format)
        {
            case VK_FORMAT_R8_UNORM:
                return "VK_FORMAT_R8_UNORM";
            case VK_FORMAT_R8G8B8_UNORM:
                return "VK_FORMAT_R8G8B8_UNORM";
            case VK_FORMAT_R8G8B8A8_UNORM:
                return "VK_FORMAT_R8G8B8A8_UNORM";
            case VK_FORMAT_B8G8R8_UNORM:
                return "VK_FORMAT_B8G8R8_UNORM";
            case VK_FORMAT_B8G8R8A8_UNORM:
                return "VK_FORMAT_B8G8R8A8_UNORM";
            case VK_FORMAT_D16_UNORM:
                return "VK_FORMAT_D16_UNORM";
            case VK_FORMAT_D32_SFLOAT:
                return "VK_FORMAT_D32_SFLOAT";
            case VK_FORMAT_D16_UNORM_S8_UINT:
                return "VK_FORMAT_D16_UNORM_S8_UINT";
            case VK_FORMAT_D24_UNORM_S8_UINT:
                return "VK_FORMAT_D24_UNORM_S8_UINT";
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                return "VK_FORMAT_D32_SFLOAT_S8_UINT";
            case VK_FORMAT_R32_SFLOAT:
                return "VK_FORMAT_R32_SFLOAT";
            case VK_FORMAT_R32G32_SFLOAT:
                return "VK_FORMAT_R32G32_SFLOAT";
            case VK_FORMAT_R32G32B32_SFLOAT:
                return "VK_FORMAT_R32G32B32_SFLOAT";
            case VK_FORMAT_R32G32B32A32_SFLOAT:
                return "VK_FORMAT_R32G32B32A32_SFLOAT";
        }

    check(0);
    return "VK_FORMAT_UNDEFINED";
}

inline VkFormat
convertFormat(const Fox::EFormat format)
{
    switch (format)
        {
            case Fox::EFormat::R8_UNORM:
                return VK_FORMAT_R8_UNORM;
            case Fox::EFormat::R8G8B8_UNORM:
                return VK_FORMAT_R8G8B8_UNORM;
            case Fox::EFormat::R8G8B8A8_UNORM:
                return VK_FORMAT_R8G8B8A8_UNORM;
            case Fox::EFormat::B8G8R8_UNORM:
                return VK_FORMAT_B8G8R8_UNORM;
            case Fox::EFormat::B8G8R8A8_UNORM:
                return VK_FORMAT_B8G8R8A8_UNORM;
            case Fox::EFormat::DEPTH16_UNORM:
                return VK_FORMAT_D16_UNORM;
            case Fox::EFormat::DEPTH32_FLOAT:
                return VK_FORMAT_D32_SFLOAT;
            case Fox::EFormat::DEPTH16_UNORM_STENCIL8_UINT:
                return VK_FORMAT_D16_UNORM_S8_UINT;
            case Fox::EFormat::DEPTH24_UNORM_STENCIL8_UINT:
                return VK_FORMAT_D24_UNORM_S8_UINT;
            case Fox::EFormat::DEPTH32_FLOAT_STENCIL8_UINT:
                return VK_FORMAT_D32_SFLOAT_S8_UINT;
            case Fox::EFormat::R32_FLOAT:
                return VK_FORMAT_R32_SFLOAT;
            case Fox::EFormat::R32G32_FLOAT:
                return VK_FORMAT_R32G32_SFLOAT;
            case Fox::EFormat::R32G32B32_FLOAT:
                return VK_FORMAT_R32G32B32_SFLOAT;
            case Fox::EFormat::R32G32B32A32_FLOAT:
                return VK_FORMAT_R32G32B32A32_SFLOAT;
        }

    check(0);
    return VK_FORMAT_UNDEFINED;
};

inline VkSampleCountFlagBits
convertVkSampleCount(const Fox::ESampleBit sample)
{

    switch (sample)
        {
            case Fox::ESampleBit::COUNT_1_BIT:
                return VK_SAMPLE_COUNT_1_BIT;
            case Fox::ESampleBit::COUNT_2_BIT:
                return VK_SAMPLE_COUNT_2_BIT;
            case Fox::ESampleBit::COUNT_4_BIT:
                return VK_SAMPLE_COUNT_4_BIT;
            case Fox::ESampleBit::COUNT_8_BIT:
                return VK_SAMPLE_COUNT_8_BIT;
            case Fox::ESampleBit::COUNT_16_BIT:
                return VK_SAMPLE_COUNT_16_BIT;
            case Fox::ESampleBit::COUNT_32_BIT:
                return VK_SAMPLE_COUNT_32_BIT;
            case Fox::ESampleBit::COUNT_64_BIT:
                return VK_SAMPLE_COUNT_64_BIT;
        }

    check(0);
    return VK_SAMPLE_COUNT_1_BIT;
}

inline VkAttachmentLoadOp
convertAttachmentLoadOp(const Fox::ERenderPassLoad load)
{
    switch (load)
        {
            case Fox::ERenderPassLoad::Clear:
                return VK_ATTACHMENT_LOAD_OP_CLEAR;
            case Fox::ERenderPassLoad::Load:
                return VK_ATTACHMENT_LOAD_OP_LOAD;
        }

    check(0);
    return VK_ATTACHMENT_LOAD_OP_CLEAR;
}

inline VkAttachmentStoreOp
convertAttachmentStoreOp(const Fox::ERenderPassStore store)
{
    switch (store)
        {
            case Fox::ERenderPassStore::DontCare:
                return VK_ATTACHMENT_STORE_OP_DONT_CARE;
            case Fox::ERenderPassStore::Store:
                return VK_ATTACHMENT_STORE_OP_STORE;
        }

    check(0);
    return VK_ATTACHMENT_STORE_OP_STORE;
}

inline bool
isColorFormat(const VkFormat format)
{
    switch (format)
        {
            case VK_FORMAT_D16_UNORM:
            case VK_FORMAT_X8_D24_UNORM_PACK32:
            case VK_FORMAT_D32_SFLOAT:
            case VK_FORMAT_S8_UINT:
            case VK_FORMAT_D16_UNORM_S8_UINT:
            case VK_FORMAT_D24_UNORM_S8_UINT:
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                return false;
        }

    return true;
}

inline VkImageLayout
convertRenderPassLayout(const Fox::ERenderPassLayout layout, bool isColor = true)
{
    switch (layout)
        {
            case Fox::ERenderPassLayout::Undefined:
                return VK_IMAGE_LAYOUT_UNDEFINED;
            case Fox::ERenderPassLayout::AsAttachment:
                if (isColor)
                    {
                        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    }
                else
                    {
                        return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
                    }
            case Fox::ERenderPassLayout::ShaderReadOnly:
                return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            case Fox::ERenderPassLayout::Present:
                return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }

    check(0);
    return VK_IMAGE_LAYOUT_UNDEFINED;
}

inline std::vector<const char*>
convertLayerPropertiesToNames(const std::vector<VkLayerProperties>& layers)
{
    std::vector<const char*> names;

    std::transform(layers.cbegin(), layers.cend(), std::back_inserter(names), [](const VkLayerProperties& p) { return p.layerName; });

    return names;
};

inline std::vector<const char*>
convertExtensionPropertiesToNames(const std::vector<VkExtensionProperties>& layers)
{
    std::vector<const char*> names;

    std::transform(layers.cbegin(), layers.cend(), std::back_inserter(names), [](const VkExtensionProperties& p) { return p.extensionName; });

    return names;
};

inline std::vector<VkLayerProperties>
getInstanceLayerProperties()
{
    // Get supported layers by vulkan
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    return availableLayers;
};

inline std::vector<VkExtensionProperties>
getInstanceExtensionProperties()
{
    // Get supported layers by vulkan
    uint32_t extensionCount;
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, availableExtensions.data());

    return availableExtensions;
};

inline std::vector<VkExtensionProperties>
getDeviceExtensionProperties(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, "", &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, "", &extensionCount, availableExtensions.data());
    return availableExtensions;
};

inline std::vector<VkLayerProperties>
getDeviceLayerProperties(VkPhysicalDevice device)
{
    // Get supported layers by vulkan
    uint32_t layerCount = 0;
    vkEnumerateDeviceLayerProperties(device, &layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateDeviceLayerProperties(device, &layerCount, availableLayers.data());
    return availableLayers;
};

inline VkResult
enumeratePhysicalDevices(VkInstance vkInstance, std::vector<VkPhysicalDevice>& physicalDevices)
{
    physicalDevices.clear();

    uint32_t deviceCount = 0;
    // Get number of available physical devices
    {
        const VkResult result = vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);
        if (VKFAILED(result))
            {
                return result;
            }
    }

    if (deviceCount > 0)
        {
            // Enumerate the physical devices
            physicalDevices.resize(deviceCount);

            const VkResult result = vkEnumeratePhysicalDevices(vkInstance, &deviceCount, physicalDevices.data());
            if (VKFAILED(result))
                {
                    return result;
                }
        }

    return VkResult::VK_SUCCESS;
};

inline uint32_t
selectPhysycalDeviceOnHighestMemory(const std::vector<VkPhysicalDevice>& physicalDevices)
{
    // Select physical device to be used for the Vulkan example
    uint32_t selectedDevice = 0;

    // Multi-GPU - Select the device with highest memory available (usually the best gpu, but not always)
    if (physicalDevices.size() > 1)
        {
            uint64_t highestMemoryOfSelectedDevice = 0;
            for (size_t i = 0; i < physicalDevices.size(); i++)
                {
                    VkPhysicalDevice                 device       = physicalDevices[i];
                    VkPhysicalDeviceMemoryProperties deviceMemory = {};
                    vkGetPhysicalDeviceMemoryProperties(device, &deviceMemory);

                    VkPhysicalDeviceProperties prop{};
                    vkGetPhysicalDeviceProperties(device, &prop);
                    const std::string deviceName = prop.deviceName;

                    const std::string out = "Gpu index:" + std::to_string(i) + " with name:" + deviceName;

                    if (deviceMemory.memoryHeaps->size > highestMemoryOfSelectedDevice)
                        {
                            selectedDevice                = (uint32_t)i;
                            highestMemoryOfSelectedDevice = deviceMemory.memoryHeaps->size;
                        }
                }
        }
    return selectedDevice;
};

//@TODO can be unit tested
inline std::vector<const char*>
filterInclusive(const std::vector<const char*>& source, const std::vector<const char*>& included)
{
    std::vector<const char*> inclusive;
    std::copy_if(source.cbegin(), source.cend(), std::back_inserter(inclusive), [&included](const char* s1) {
        const auto found = std::find_if(included.begin(), included.end(), [s1](const char* s2) { return (strcmp(s1, s2) == 0); });
        return found != included.end();
    });

    return inclusive;
};

//@TODO can be unit tested
inline std::vector<const char*>
filterExclusive(const std::vector<const char*>& source, const std::vector<const char*>& excluded)
{
    std::vector<const char*> inclusive;
    std::copy_if(source.cbegin(), source.cend(), std::back_inserter(inclusive), [&excluded](const char* s1) {
        const auto found = std::find_if(excluded.begin(), excluded.end(), [s1](const char* s2) { return (strcmp(s1, s2) == 0); });
        return found == excluded.end();
    });

    return inclusive;
};

inline VkImageLayout
convertAttachmentReferenceLayout(const Fox::EAttachmentReference& att)
{
    switch (att)
        {
            case Fox::EAttachmentReference::COLOR_READ_ONLY:
                return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            case Fox::EAttachmentReference::COLOR_ATTACHMENT:
                return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            case Fox::EAttachmentReference::DEPTH_STENCIL_READ_ONLY:
                return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
            case Fox::EAttachmentReference::DEPTH_STENCIL_ATTACHMENT:
                return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

    check(0);
    return VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
}

//@TODO add unit test
inline std::vector<VkDescriptorSetLayoutBinding>
convertDescriptorBindings(const std::map<uint32_t /*binding*/, ::Fox::ShaderDescriptorBindings>& bindingToDescription)
{

    std::vector<VkDescriptorSetLayoutBinding> bindings;

    for (const auto& pair : bindingToDescription)
        {
            const auto  binding     = pair.first;
            const auto& description = pair.second;

            VkDescriptorSetLayoutBinding b{};
            b.binding            = binding;
            b.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            b.descriptorCount    = description.Count;
            b.stageFlags         = VK_SHADER_STAGE_ALL_GRAPHICS;
            b.pImmutableSamplers = nullptr;

            switch (description.StorageType)
                {
                    case ::Fox::EBindingType::UNIFORM_BUFFER_OBJECT:
                        b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        break;
                    // case RIShaderDescriptorBindings::Type::DYNAMIC:
                    //     b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                    //     break;
                    case ::Fox::EBindingType::STORAGE_BUFFER_OBJECT:
                        b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                        break;
                    case ::Fox::EBindingType::SAMPLER:
                        b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        break;
                }

            switch (description.Stage)
                {
                    case ::Fox::EShaderStage::VERTEX:
                        b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                        break;
                    case ::Fox::EShaderStage::FRAGMENT:
                        b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                        break;
                    case ::Fox::EShaderStage::ALL:
                        b.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
                        break;
                }
            bindings.emplace_back(std::move(b));
        }

    return bindings;
}

//@TODO add unit test
inline std::vector<VkDescriptorPoolSize>
computeDescriptorSetsPoolSize(const std::map<uint32_t /*Set*/, std::map<uint32_t /*binding*/, ::Fox::ShaderDescriptorBindings>>& sets)
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (const auto& setPair : sets)
        {
            for (const auto& bindingPair : setPair.second)
                {
                    VkDescriptorType descriptorType{};
                    switch (bindingPair.second.StorageType)
                        {
                            case ::Fox::EBindingType::UNIFORM_BUFFER_OBJECT:
                                descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                                break;
                            // case RIShaderDescriptorBindings::Type::DYNAMIC:
                            //     descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                            //     break;
                            case ::Fox::EBindingType::STORAGE_BUFFER_OBJECT:
                                descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                                break;
                            case ::Fox::EBindingType::SAMPLER:
                                descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                                break;
                        }

                    auto sameTypeIt = std::find_if(poolSizes.begin(), poolSizes.end(), [&descriptorType](const VkDescriptorPoolSize& size) { return size.type == descriptorType; });
                    if (sameTypeIt == poolSizes.end())
                        {
                            VkDescriptorPoolSize poolSize{ descriptorType, bindingPair.second.Count };
                            poolSizes.push_back(poolSize);
                        }
                    else
                        {
                            sameTypeIt->descriptorCount += bindingPair.second.Count;
                        }
                }
        }
    return poolSizes;
}

inline VkResult
createShaderModule(VkDevice device, const std::vector<unsigned char>& byteCode, VkShaderModule* shaderModule)
{
    check(device);
    check(byteCode.size() > 0);
    check(shaderModule && !*shaderModule);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = byteCode.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(byteCode.data());

    return vkCreateShaderModule(device, &createInfo, nullptr, shaderModule);
}

inline VkPipelineShaderStageCreateInfo
createShaderStageInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule)
{
    constexpr const char* PENTRYPOINTNAME{ "main" };
    check(shaderModule);
    check(stage == VK_SHADER_STAGE_VERTEX_BIT || stage == VK_SHADER_STAGE_FRAGMENT_BIT);

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.pNext               = nullptr;
    stageInfo.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.flags               = NULL;
    stageInfo.stage               = stage;
    stageInfo.module              = shaderModule;
    stageInfo.pName               = PENTRYPOINTNAME;
    stageInfo.pSpecializationInfo = nullptr;

    return stageInfo;
}

inline void
fullPipelineBarrier(VkCommandBuffer cmd)
{
    VkMemoryBarrier barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER, NULL, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT };
    vkCmdPipelineBarrier(cmd,
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, // Source pipeline stage
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, // Destination pipeline stage
    0, // Dependency flags
    1,
    &barrier, // Memory barriers
    0,
    nullptr, // Buffer memory barriers
    0,
    0 // Image memory barriers (if applicable)
    );
}

}