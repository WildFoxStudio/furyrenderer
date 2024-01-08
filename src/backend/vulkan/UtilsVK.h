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
            case VK_ERROR_SURFACE_LOST_KHR:
                return "VK_ERROR_SURFACE_LOST_KHR";
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
            case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
                return Fox::EFormat::RGBA_DXT1;
            case VK_FORMAT_BC3_UNORM_BLOCK:
                return Fox::EFormat::RGBA_DXT3;
            case VK_FORMAT_BC5_UNORM_BLOCK:
                return Fox::EFormat::RGBA_DXT5;
            case VK_FORMAT_R32_SINT:
                return Fox::EFormat::SINT32;
        }

    check(0);
    return Fox::EFormat::R8_UNORM;
};

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
            case Fox::EFormat::RGBA_DXT1:
                return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
            case Fox::EFormat::RGBA_DXT3:
                return VK_FORMAT_BC3_UNORM_BLOCK;
            case Fox::EFormat::RGBA_DXT5:
                return VK_FORMAT_BC5_UNORM_BLOCK;
            case Fox::EFormat::SINT32:
                return VK_FORMAT_R32_SINT;
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

inline bool
formatHasStencil(const VkFormat format)
{
    switch (format)
        {
            case VK_FORMAT_S8_UINT:
            case VK_FORMAT_D16_UNORM_S8_UINT:
            case VK_FORMAT_D24_UNORM_S8_UINT:
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                return true;
        }

    return false;
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
                    case ::Fox::EBindingType::TEXTURE:
                        b.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                        break;
                    case ::Fox::EBindingType::SAMPLER:
                        b.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
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

inline VkPipelineStageFlags
determinePipelineStageFlags(VkAccessFlags accessFlags, Fox::EQueueType queueType)
{
    // This function was copied from TheForge rendering framework
    VkPipelineStageFlags flags = 0;

    switch (queueType)
        {
            case Fox::EQueueType::QUEUE_GRAPHICS:
                {
                    if ((accessFlags & (VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT)) != 0)
                        flags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;

                    if ((accessFlags & (VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT)) != 0)
                        {
                            flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
                            flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                            // if (pRenderer->pActiveGpuSettings->mGeometryShaderSupported)
                            //     {
                            //         flags |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
                            //     }
                            // if (pRenderer->pActiveGpuSettings->mTessellationSupported)
                            //     {
                            //         flags |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
                            //         flags |= VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
                            //     }
                            flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
#ifdef VK_RAYTRACING_AVAILABLE
                            if (pRenderer->mVulkan.mRaytracingSupported)
                                {
                                    flags |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
                                }
#endif
                        }

                    if ((accessFlags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT) != 0)
                        flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

                    if ((accessFlags & (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)) != 0)
                        flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

                    if ((accessFlags & (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)) != 0)
                        flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

#if defined(QUEST_VR)
                    if ((accessFlags & VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT) != 0)
                        flags |= VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
#endif
                    break;
                }
            case Fox::EQueueType::QUEUE_COMPUTE:
                {
                    if ((accessFlags & (VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT)) != 0 || (accessFlags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT) != 0 ||
                    (accessFlags & (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)) != 0 ||
                    (accessFlags & (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)) != 0)
                        return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

                    if ((accessFlags & (VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT)) != 0)
                        flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

                    break;
                }
            case Fox::EQueueType::QUEUE_TRANSFER:
                return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            default:
                break;
        }

    // Compatible with both compute and graphics queues
    if ((accessFlags & VK_ACCESS_INDIRECT_COMMAND_READ_BIT) != 0)
        flags |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;

    if ((accessFlags & (VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT)) != 0)
        flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;

    if ((accessFlags & (VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT)) != 0)
        flags |= VK_PIPELINE_STAGE_HOST_BIT;

    if (flags == 0)
        flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    return flags;
}

inline VkImageLayout
resourceStateToImageLayout(Fox::EResourceState usage)
{
    if ((uint32_t)usage & (uint32_t)Fox::EResourceState::COPY_SOURCE)
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    if ((uint32_t)usage & (uint32_t)Fox::EResourceState::COPY_DEST)
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    if ((uint32_t)usage & (uint32_t)Fox::EResourceState::RENDER_TARGET)
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    if ((uint32_t)usage & (uint32_t)Fox::EResourceState::DEPTH_WRITE)
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    if ((uint32_t)usage & (uint32_t)Fox::EResourceState::UNORDERED_ACCESS)
        return VK_IMAGE_LAYOUT_GENERAL;

    if ((uint32_t)usage & (uint32_t)Fox::EResourceState::SHADER_RESOURCE)
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    if ((uint32_t)usage & (uint32_t)Fox::EResourceState::PRESENT)
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    if ((uint32_t)usage == (uint32_t)Fox::EResourceState::COMMON)
        return VK_IMAGE_LAYOUT_GENERAL;

#if defined(QUEST_VR)
    if ((uint32_t)usage == (uint32_t)Fox::EResourceState::SHADING_RATE_SOURCE)
        return VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
#endif

    return VK_IMAGE_LAYOUT_UNDEFINED;
}

inline VkAccessFlags
resourceStateToAccessFlag(Fox::EResourceState state)
{
    VkAccessFlags ret = 0;
    if ((uint32_t)state & (uint32_t)Fox::EResourceState::COPY_SOURCE)
        {
            ret |= VK_ACCESS_TRANSFER_READ_BIT;
        }
    if ((uint32_t)state & (uint32_t)Fox::EResourceState::COPY_DEST)
        {
            ret |= VK_ACCESS_TRANSFER_WRITE_BIT;
        }
    if ((uint32_t)state & (uint32_t)Fox::EResourceState::VERTEX_AND_CONSTANT_BUFFER)
        {
            ret |= VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        }
    if ((uint32_t)state & (uint32_t)Fox::EResourceState::INDEX_BUFFER)
        {
            ret |= VK_ACCESS_INDEX_READ_BIT;
        }
    if ((uint32_t)state & (uint32_t)Fox::EResourceState::UNORDERED_ACCESS)
        {
            ret |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        }
    if ((uint32_t)state & (uint32_t)Fox::EResourceState::INDIRECT_ARGUMENT)
        {
            ret |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        }
    if ((uint32_t)state & (uint32_t)Fox::EResourceState::RENDER_TARGET)
        {
            ret |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        }
    if ((uint32_t)state & (uint32_t)Fox::EResourceState::DEPTH_WRITE)
        {
            ret |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        }
    if ((uint32_t)state & (uint32_t)Fox::EResourceState::SHADER_RESOURCE)
        {
            ret |= VK_ACCESS_SHADER_READ_BIT;
        }
    if ((uint32_t)state & (uint32_t)Fox::EResourceState::PRESENT)
        {
            ret |= VK_ACCESS_MEMORY_READ_BIT;
        }
#if defined(QUEST_VR)
    if ((uint32_t)state & (uint32_t)Fox::EResourceState::SHADING_RATE_SOURCE)
        {
            ret |= VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT;
        }
#endif

#ifdef VK_RAYTRACING_AVAILABLE
    if ((uint32_t)state & (uint32_t)Fox::EResourceState::RAYTRACING_ACCELERATION_STRUCTURE)
        {
            ret |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        }
#endif
    return ret;
}

#define CASE(FIRST, SECOND) \
    case FIRST: \
        return SECOND;

inline VkImageLayout
convertResourceStateToImageLayout(Fox::EResourceState state, bool isDepth)
{
    using namespace Fox;
    // UNDEFINED                         = 0,
    // VERTEX_AND_CONSTANT_BUFFER        = 0x1,
    // INDEX_BUFFER                      = 0x2,
    // RENDER_TARGET                     = 0x4,
    // UNORDERED_ACCESS                  = 0x8,
    // DEPTH_WRITE                       = 0x10,
    // DEPTH_READ                        = 0x20,
    // NON_PIXEL_SHADER_RESOURCE         = 0x40,
    // PIXEL_SHADER_RESOURCE             = 0x80,
    // SHADER_RESOURCE                   = 0x40 | 0x80,
    // STREAM_OUT                        = 0x100,
    // INDIRECT_ARGUMENT                 = 0x200,
    // COPY_DEST                         = 0x400,
    // COPY_SOURCE                       = 0x800,
    // GENERAL_READ                      = (((((0x1 | 0x2) | 0x40) | 0x80) | 0x200) | 0x800),
    // PRESENT                           = 0x1000,
    // COMMON                            = 0x2000,
    // RAYTRACING_ACCELERATION_STRUCTURE = 0x4000,
    // SHADING_RATE_SOURCE               = 0x8000,

    switch (state)
        {
            case EResourceState::UNDEFINED:
                return VK_IMAGE_LAYOUT_UNDEFINED;

            case EResourceState::RENDER_TARGET:
                return (!isDepth) ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            case EResourceState::COPY_DEST:
                return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

            case EResourceState::COPY_SOURCE:
                return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

            case EResourceState::PRESENT:
                return VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR;

            default:
                break;
        }

    check(0);
    return VK_IMAGE_LAYOUT_UNDEFINED;
}

inline VkFlags
convertQueueTypeToVkFlags(uint32_t queueTypeFlag)
{
    using namespace Fox;
    uint32_t vkFlags{};
    if (queueTypeFlag & EQueueType::QUEUE_GRAPHICS)
        {
            vkFlags |= VK_QUEUE_GRAPHICS_BIT;
        }
    if (queueTypeFlag & EQueueType::QUEUE_COMPUTE)
        {
            vkFlags |= VK_QUEUE_COMPUTE_BIT;
        }
    if (queueTypeFlag & EQueueType::QUEUE_TRANSFER)
        {
            vkFlags |= VK_QUEUE_TRANSFER_BIT;
        }

    return vkFlags;
}

/**
 * @brief Return a graphics flagged queue if requestedFlags is only graphics. If requested flags is only transfer or only compute it returns a dedicated queue with that flag only if available on
 * hardware otherwise will return a non dedicated queue with also other flags. If no dedicated queue is found it will return the first queue that supports the requested flags;
 * @param device The vulkan device
 * @param outFamilyIndex out the family index found
 * @param outQueueIndex out the queue index to use
 * @param requestedFlags the queue flags requested
 * @param queueFamilyPropertiesPtr a pointer to an array of VkQueueFamilyProperties
 * @param queueFamilyPropertiesCount number of elements in queueFamilyPropertiesPtr
 * @param queueFamilyIndexCreatedCount Array of int32 to count how many queues are used for each family. Length must be at least queueFamilyPropertiesCount
 * @return The queue family index the has at least the requestedFlags.
 */
inline bool
findQueueWithFlags(uint32_t*         outFamilyIndex,
uint32_t*                            outQueueIndex,
VkFlags                              requestedFlags,
const VkQueueFamilyProperties* const queueFamilyPropertiesPtr,
uint32_t                             queueFamilyPropertiesCount,
uint32_t*                            queueFamilyIndexCreatedCount)
{
    using namespace Fox;

    bool found{ false };
    *outFamilyIndex = 0;
    *outQueueIndex  = 0;

    for (uint32_t i = 0; i < queueFamilyPropertiesCount; i++)
        {
            const VkQueueFamilyProperties* const queueFamily = queueFamilyPropertiesPtr + i;
            const bool                           isGraphics  = (queueFamily->queueFlags & VK_QUEUE_GRAPHICS_BIT) ? true : false;
            // If has the required flags but is also graphics and has unused queues
            if (requestedFlags & VK_QUEUE_GRAPHICS_BIT && isGraphics)
                {
                    found           = true;
                    *outFamilyIndex = i;
                    // User can query as many graphics queue as it can, we will return always the same since there is no benefit of using multiple ones
                    break;
                }
            // if requested is specialized queue with only the flag requested
            if (queueFamily->queueFlags & requestedFlags && (queueFamily->queueFlags & ~requestedFlags) == 0 && queueFamilyIndexCreatedCount[i] < queueFamily->queueCount)
                {
                    found           = true;
                    *outFamilyIndex = i;
                    *outQueueIndex  = queueFamilyIndexCreatedCount[i]++;

                    break;
                }
            uint32_t orRequestedFlags = (queueFamily->queueFlags & requestedFlags);
            // Should return a queue with requestedFlag but is not general purpose nor specialized
            if (queueFamily->queueFlags & orRequestedFlags && !isGraphics && queueFamilyIndexCreatedCount[i] < queueFamily->queueCount)
                {
                    found           = true;
                    *outFamilyIndex = i;
                    *outQueueIndex  = queueFamilyIndexCreatedCount[i]++;
                    break;
                }
        }

    // Find a any queue with the requested queueTypeFlags
    if (!found)
        {
            for (uint32_t i = 0; i < queueFamilyPropertiesCount; i++)
                {
                    const VkQueueFamilyProperties* const queueFamily = queueFamilyPropertiesPtr + i;
                    if (queueFamily->queueFlags & requestedFlags)
                        {
                            found           = true;
                            *outFamilyIndex = i;
                            break;
                        }
                }
        }

    check(found == true);
    return found;
}

}