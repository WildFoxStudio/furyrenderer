// Copyright RedFox Studio 2022

#include "RIFactoryVk.h"

#if defined(FOX_USE_VULKAN)

#include "Core/utils/Filter.h"
#include <iostream>

#if defined(FOX_WINDOWS)
#include "Window/windows/Win32.h"
#define VK_USE_PLATFORM_WIN32_KHR
#else
#include "Window/linux/WindowSDL.h"
#define VK_USE_PLATFORM_XLIB_KHR
// s#include <vulkan/vulkan_xlib.h>
#endif

namespace Fox
{

RIFactoryVk::RIFactoryVk()
{

    _cleanupResources.push_back([this]() {
        for (const auto& [hash, renderPass] : _cachedRenderPass)
            {
                MT_DestroyRenderPass(renderPass);
            }
    });
    _cleanupResources.push_back([this]() {
        for (VkDescriptorPool ptr : _genericDescriptorPools)
            {
                VulkanDevice.DestroyDescriptorPool(ptr);
            }
        _genericDescriptorPools.clear();
    });
    _cleanupResources.push_back([this]() { _cachedDescriptorSetPool.clear(); });

    _cleanupResources.push_back([this]() { VulkanDevice.DestroyBuffer(_uboSsboBufferPool); });
}

RIFactoryVk::~RIFactoryVk() { std::cerr << "Tearing Vulkan Factory\n" << std::flush; }

RIDescriptorSetBinder
RIFactoryVk::CreateDescriptorSetBinder()
{
    return _globalDescriptorPool->CreateDescriptorSetBinder();
    // return VulkanDevice.CreateDescriptorSetBinder(_globalDescriptorPool);
}

std::unique_ptr<RIDescriptorPoolManager>
RIFactoryVk::CreateDescriptorPoolManager(VkDescriptorPool pool, uint32_t maxSets)
{
    return std::make_unique<RIDescriptorPoolManager>(VulkanDevice.Device, pool, maxSets);
}

VkSurfaceKHR
RIFactoryVk::MT_CreateSurfaceFromWindow(AWindow* window)
{
    return VulkanInstance.CreateSurfaceFromWindow(window);
}

void
RIFactoryVk::MT_DestroySurface(VkSurfaceKHR surface)
{
    VulkanInstance.DestroySurface(surface);
}

VkSwapchainKHR
RIFactoryVk::MT_CreateSwapchainFromSurface(VkSurfaceKHR surface, VkSurfaceFormatKHR& format, VkPresentModeKHR& presentMode)
{
    const bool supportPresentation = VulkanDevice.SurfaceSupportPresentationOnCurrentQueueFamily(surface);
    critical(supportPresentation);

    VkSurfaceCapabilitiesKHR capabilities = MT_GetSurfaceCapabilities(surface);

    return VulkanDevice.CreateSwapchainFromSurface(surface, format, presentMode, capabilities);
}

VkSwapchainKHR
RIFactoryVk::MT_RecreateSwapchainFromOld(VkSurfaceKHR surface, VkSwapchainKHR oldSwapchain, VkSurfaceFormatKHR format, VkPresentModeKHR presentMode)
{
    const bool supportPresentation = VulkanDevice.SurfaceSupportPresentationOnCurrentQueueFamily(surface);
    critical(supportPresentation);

    VkSurfaceCapabilitiesKHR capabilities = MT_GetSurfaceCapabilities(surface);

    return VulkanDevice.CreateSwapchainFromSurface(surface, format, presentMode, capabilities, oldSwapchain);
}

void
RIFactoryVk::MT_DestroySwapchain(VkSwapchainKHR swapchain)
{
    vkDeviceWaitIdle(VulkanDevice.Device);
    VulkanDevice.DestroySwapchain(swapchain);
}

void
RIFactoryVk::MT_GetSwapchainImages(VkSwapchainKHR swapchain, std::vector<VkImage>& images)
{
    images = VulkanDevice.GetSwapchainImages(swapchain);
}

VkImageView
RIFactoryVk::MT_CreateImageView(VkImage image, VkFormat format)
{
    return VulkanDevice.CreateImageView(format, { image }, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1);
}

void
RIFactoryVk::MT_DestroyImageView(VkImageView imageView)
{
    VulkanDevice.DestroyImageView(imageView);
}

VkSemaphore
RIFactoryVk::MT_CreateSemaphore()
{
    return VulkanDevice.CreateVkSemaphore();
}

void
RIFactoryVk::MT_DestroySemaphore(VkSemaphore semaphore)
{
    VulkanDevice.DestroyVkSemaphore(semaphore);
}

VkFramebuffer
RIFactoryVk::MT_CreateFramebuffer(const std::vector<VkImageView> imageViews, VkExtent2D extent, VkRenderPass renderPass)
{
    return VulkanDevice.CreateFramebuffer(imageViews, extent.width, extent.height, renderPass);
}

void
RIFactoryVk::MT_DestroyFramebuffer(VkFramebuffer framebuffer)
{
    VulkanDevice.DestroyFramebuffer(framebuffer);
}

VkRenderPass
RIFactoryVk::MT_CreateRenderPass(const DRenderPassAttachments& attachments)
{
    const auto found = _cachedRenderPass.find(attachments);
    if (found != _cachedRenderPass.end())
        {
            return found->second;
        }

    std::vector<VkAttachmentDescription> attachmentDescription;
    attachmentDescription.reserve(attachments.Attachments.size());

    std::vector<VkAttachmentReference> colorAttachmentReference;
    colorAttachmentReference.reserve(attachments.Attachments.size());

    std::vector<VkAttachmentReference> depthStencilAttachmentReference;
    depthStencilAttachmentReference.reserve(attachments.Attachments.size());

    for (const DRenderPassAttachment& att : attachments.Attachments)
        {
            VkAttachmentReference ref{};
            check(attachmentDescription.size() < std::numeric_limits<uint32_t>::max());

            ref.attachment = (uint32_t)attachmentDescription.size();
            ref.layout     = att.GetAttachmentReferenceLayout();

            switch (ref.layout)
                {
                    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                        colorAttachmentReference.emplace_back(ref);
                        break;
                    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
                    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
                        depthStencilAttachmentReference.emplace_back(ref);
                        break;
                    default:
                        check(0);
                        break;
                }

            VkAttachmentDescription desc{};
            desc.format  = att.GetFormat();
            desc.samples = att.GetSamplesCount();
            desc.loadOp  = att.GetLoadOp();
            desc.storeOp = att.GetStoreOp();
            // Stencil not used right now
            desc.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

            desc.initialLayout = att.GetInitialLayout();
            desc.finalLayout   = att.GetFinalLayout();

            attachmentDescription.emplace_back(desc);
        }

    VkSubpassDescription subpass{};
    subpass.flags                   = 0;
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.inputAttachmentCount    = 0;
    subpass.colorAttachmentCount    = (uint32_t)colorAttachmentReference.size();
    subpass.pColorAttachments       = colorAttachmentReference.data();
    subpass.pResolveAttachments     = NULL;
    subpass.pDepthStencilAttachment = (depthStencilAttachmentReference.size()) ? depthStencilAttachmentReference.data() : nullptr;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments    = NULL;

    VkSubpassDependency dependency{};
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.pNext           = NULL;
    renderPassInfo.attachmentCount = (uint32_t)attachmentDescription.size();
    renderPassInfo.pAttachments    = attachmentDescription.data();
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies   = &dependency;

    {
        VkRenderPass   renderPass{};
        const VkResult result = vkCreateRenderPass(VulkanDevice.Device, &renderPassInfo, nullptr, &renderPass);
        if (VKFAILED(result))
            {
                throw std::runtime_error(VkErrorString(result));
            }

        _cachedRenderPass[attachments] = renderPass;
        return renderPass;
    }
}

void
RIFactoryVk::MT_DestroyRenderPass(VkRenderPass renderPass)
{
    vkDestroyRenderPass(VulkanDevice.Device, renderPass, nullptr);
}

bool
RIFactoryVk::MT_SurfaceSupportPresentationOnCurrentQueueFamily(VkSurfaceKHR surface)
{
    VkBool32       isSupported = VK_FALSE;
    const VkResult result      = RICoreVk::GetPhysicalDeviceSupportSurfaceKHR(VulkanDevice.PhysicalDevice, VulkanDevice.GetQueueFamilyIndex(), surface, &isSupported);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkErrorString(result));
        }
    return isSupported == VK_TRUE ? true : false;
}

std::vector<VkSurfaceFormatKHR>
RIFactoryVk::MT_GetSurfaceFormats(VkSurfaceKHR surface)
{
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    {
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(VulkanDevice.PhysicalDevice, surface, &formatCount, nullptr);

        if (formatCount != 0)
            {
                surfaceFormats.resize(formatCount);
                const VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(VulkanDevice.PhysicalDevice, surface, &formatCount, surfaceFormats.data());
                if (VKFAILED(result))
                    {
                        throw std::runtime_error(VkErrorString(result));
                    }
            }
    }

    return surfaceFormats;
}

VkSurfaceCapabilitiesKHR
RIFactoryVk::MT_GetSurfaceCapabilities(VkSurfaceKHR surface)
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
    {
        const VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VulkanDevice.PhysicalDevice, surface, &surfaceCapabilities);
        if (VKFAILED(result))
            {
                throw std::runtime_error(VkErrorString(result));
            }
    }

    return surfaceCapabilities;
}

std::vector<VkPresentModeKHR>
RIFactoryVk::MT_GetSurfacePresentModes(VkSurfaceKHR surface)
{
    std::vector<VkPresentModeKHR> presentModes;
    {
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(VulkanDevice.PhysicalDevice, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0)
            {
                presentModes.resize(presentModeCount);
                const VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(VulkanDevice.PhysicalDevice, surface, &presentModeCount, presentModes.data());
                if (VKFAILED(result))
                    {
                        throw std::runtime_error(VkErrorString(result));
                    }
            }
    }
    return presentModes;
}

//@TODO REMOVE IT IS DEPRECATED
VkDescriptorSetLayout
RIFactoryVk::MT_CreateWorldDescriptorSetLayout()
{
    std::vector<RIShaderDescriptorBindings> layoutBindings = {
        RIShaderDescriptorBindings("ProjectionMatrix", RIShaderDescriptorBindings::Type::DYNAMIC, sizeof(glm::mat4), 1, 0, 0, ERIShaderStage::ALL),
    };

    VkDescriptorSetLayoutBinding modelLayoutBinding{};
    modelLayoutBinding.binding            = 0;
    modelLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    modelLayoutBinding.descriptorCount    = 1;
    modelLayoutBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    modelLayoutBinding.pImmutableSamplers = nullptr; // Optional only for images

    // Create descriptor set layout
    VkDescriptorSetLayoutCreateInfo _layoutCreateInfo{};
    _layoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    _layoutCreateInfo.bindingCount = 1;
    _layoutCreateInfo.pBindings    = &modelLayoutBinding;

    VkDescriptorSetLayout descriptorSetLayout{};
    const VkResult        result = vkCreateDescriptorSetLayout(VulkanDevice.Device, &_layoutCreateInfo, nullptr, &descriptorSetLayout);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkErrorString(result));
        }

    return descriptorSetLayout;
}

VkDescriptorSetLayout
RIFactoryVk::MT_CreatePostProcessDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding modelLayoutBinding{};
    modelLayoutBinding.binding            = 0;
    modelLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    modelLayoutBinding.descriptorCount    = 1;
    modelLayoutBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
    modelLayoutBinding.pImmutableSamplers = nullptr; // Optional only for images

    // Create descriptor set layout
    VkDescriptorSetLayoutCreateInfo _layoutCreateInfo{};
    _layoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    _layoutCreateInfo.bindingCount = 1;
    _layoutCreateInfo.pBindings    = &modelLayoutBinding;

    VkDescriptorSetLayout descriptorSetLayout{};
    const VkResult        result = vkCreateDescriptorSetLayout(VulkanDevice.Device, &_layoutCreateInfo, nullptr, &descriptorSetLayout);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkErrorString(result));
        }

    return descriptorSetLayout;
}

VkDescriptorSetLayout
RIFactoryVk::MT_CreateWorldMaterialDescriptorSetLayout()
{
    std::vector<RIShaderDescriptorBindings> layoutBindings = {
        RIShaderDescriptorBindings("Material", RIShaderDescriptorBindings::Type::SAMPLER_COMBINED, 0, 100, 0, 0, ERIShaderStage::FRAGMENT),
    };

    VkDescriptorSetLayoutBinding modelLayoutBinding{};
    modelLayoutBinding.binding        = 0;
    modelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    // Albedo;0
    // Normal;1
    // Occlusion;2
    // Emissive;3
    // MetallicRoughness;4
    modelLayoutBinding.descriptorCount    = 64;
    modelLayoutBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
    modelLayoutBinding.pImmutableSamplers = nullptr; // Optional only for images

    // Create descriptor set layout
    VkDescriptorSetLayoutCreateInfo _layoutCreateInfo{};
    _layoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    _layoutCreateInfo.bindingCount = 1;
    _layoutCreateInfo.pBindings    = &modelLayoutBinding;

    VkDescriptorSetLayout descriptorSetLayout{};
    const VkResult        result = vkCreateDescriptorSetLayout(VulkanDevice.Device, &_layoutCreateInfo, nullptr, &descriptorSetLayout);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkErrorString(result));
        }

    return descriptorSetLayout;
}

void
RIFactoryVk::MT_DestroyDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout)
{
    VulkanDevice.DestroyDescriptorSetLayout(descriptorSetLayout);
}

std::vector<VkPipelineShaderStageCreateInfo>
RIFactoryVk::MT_CreateShaderStages(const RIShaderByteCode& bytecode)
{
    std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfo;
    {
        {
            VkShaderModule vertex{};
            const VkResult result = RIShaderModuleBuilderVk::Create(VulkanDevice.Device, bytecode.VertexShader, &vertex);
            if (VKFAILED(result))
                {
                    throw std::runtime_error(VkErrorString(result));
                }
            shaderStageCreateInfo.push_back(RIShaderStageBuilderVk::Create(VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, vertex));
        }
        {
            VkShaderModule pixel{};
            const VkResult result = RIShaderModuleBuilderVk::Create(VulkanDevice.Device, bytecode.PixelShader, &pixel);
            if (VKFAILED(result))
                {
                    throw std::runtime_error(VkErrorString(result));
                }
            shaderStageCreateInfo.push_back(RIShaderStageBuilderVk::Create(VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, pixel));
        }
    }
    return shaderStageCreateInfo;
}

void
RIFactoryVk::MT_DestroyShaderStages(const std::vector<VkPipelineShaderStageCreateInfo>& stages)
{
    // Free shader modules
    for (const auto& stage : stages)
        {
            vkDestroyShaderModule(VulkanDevice.Device, stage.module, nullptr);
        }
}

VkPipelineLayout
RIFactoryVk::MT_CreateWorldPipelineLayout_DEPRECATED(const std::vector<VkDescriptorSetLayout>& descriptorSetLayout, const std::vector<VkPushConstantRange>& pushConstantRange)
{
    return VulkanDevice.CreatePipelineLayout(descriptorSetLayout, pushConstantRange);
}

void
RIFactoryVk::MT_DestroyPipelineLayout_DEPRECATED(VkPipelineLayout pipelineLayout)
{
    VulkanDevice.DestroyPipelineLayout(pipelineLayout);
}

std::vector<VkDescriptorSet>
RIFactoryVk::MT_CreateDescriptorSet(VkDescriptorPool descriptorPool, const std::vector<VkDescriptorSetLayout>& descriptorSetLayout)
{

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = descriptorPool;
    allocInfo.descriptorSetCount = (uint32_t)descriptorSetLayout.size();
    allocInfo.pSetLayouts        = descriptorSetLayout.data();

    std::vector<VkDescriptorSet> descriptorSet(descriptorSetLayout.size());
    const VkResult               setsResult = vkAllocateDescriptorSets(VulkanDevice.Device, &allocInfo, descriptorSet.data());
    if (VKFAILED(setsResult))
        {
            throw std::runtime_error(VkErrorString(setsResult));
        }

    return descriptorSet;
}

VkDescriptorPool
RIFactoryVk::MT_CreateDescriptorPool(const std::unordered_map<VkDescriptorType, uint32_t>& typeToCountMap, int maxSets)
{

    std::vector<VkDescriptorPoolSize> poolDimensions;
    poolDimensions.reserve(typeToCountMap.size());
    for (const auto& it : typeToCountMap)
        {
            VkDescriptorPoolSize poolSize{};
            poolSize.type            = it.first;
            poolSize.descriptorCount = it.second;
            poolDimensions.emplace_back(std::move(poolSize));
        }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags         = NULL;
    poolInfo.poolSizeCount = (uint32_t)poolDimensions.size(); // is the number of elements in pPoolSizes.
    poolInfo.pPoolSizes    = poolDimensions.data(); // is a pointer to an array of VkDescriptorPoolSize structures
    poolInfo.maxSets       = maxSets; //  is the maximum number of -descriptor sets- that can be allocated from the pool (must be grater than 0)

    VkDescriptorPool pool = VulkanDevice.CreateDescriptorPool(poolDimensions, maxSets);

    // store reference for later deletion
    _genericDescriptorPools.push_back(pool);

    return pool;
}

void
RIFactoryVk::MT_DestroyDescriptorPool(VkDescriptorPool descriptorPool)
{
    VulkanDevice.DestroyDescriptorPool(descriptorPool);
}

void
RIFactoryVk::MT_UpdateWorldCameraDescriptorSet(VkDescriptorSet descriptorSet, VkBuffer buffer, size_t bytes, uint32_t binding, size_t offset)
{
    check(offset < std::numeric_limits<uint32_t>::max());
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = (uint32_t)offset;
    bufferInfo.range  = bytes;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet           = descriptorSet;
    descriptorWrite.dstBinding       = binding;
    descriptorWrite.dstArrayElement  = 0;
    descriptorWrite.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descriptorWrite.descriptorCount  = 1;
    descriptorWrite.pImageInfo       = nullptr;
    descriptorWrite.pBufferInfo      = &bufferInfo;
    descriptorWrite.pTexelBufferView = nullptr; // Optional

    vkUpdateDescriptorSets(VulkanDevice.Device, (uint32_t)1, &descriptorWrite, 0, nullptr);
}

void
RIFactoryVk::MT_UpdateDescriptorSet(const std::vector<VkWriteDescriptorSet>& writes)
{
    vkUpdateDescriptorSets(VulkanDevice.Device, (uint32_t)writes.size(), writes.data(), 0, nullptr);
}

VkBuffer
RIFactoryVk::MT_CreateMappedBufferHostVisibile(size_t size, VkBufferUsageFlagBits usage, VmaAllocation& allocation)
{
    RIVulkanBuffer b = VulkanDevice.CreateBufferHostVisible(size, usage);
    allocation       = b.Allocation;
    return b.Buffer;
}

VkBuffer
RIFactoryVk::MT_CreateMappedBufferGpuOnly(size_t size, VkBufferUsageFlagBits usage, VmaAllocation& allocation)
{
    RIVulkanBuffer b = VulkanDevice.CreateBufferDeviceLocalTransferBit(size, usage);
    allocation       = b.Allocation;
    return b.Buffer;
}

RIVulkanImage
RIFactoryVk::MT_CreateImageGpuOnly2(int width, int height, int mipLevels, EImageFormat format, VkImageUsageFlags usage)
{
    return VulkanDevice.CreateImageDeviceLocal(width, height, mipLevels, EImageFormat_TO_VkFormat[format], usage);
}

VkImage
RIFactoryVk::MT_CreateImageGpuOnly(int width, int height, int mipLevels, EImageFormat format, VkImageUsageFlags usage, VmaAllocation& allocation)
{
    const auto img = VulkanDevice.CreateImageDeviceLocal(width, height, mipLevels, EImageFormat_TO_VkFormat[format], usage);
    allocation     = img.Allocation;
    return img.Image;
}

void
RIFactoryVk::MT_DestroyImage2(RIVulkanImage& image)
{
    VulkanDevice.DestroyImage(image);
    image.Image      = nullptr;
    image.Allocation = nullptr;
}

void
RIFactoryVk::MT_DestroyImage(VkImage image, VmaAllocation allocation)
{
    VulkanDevice.DestroyImage({ image, allocation });
}

VkBuffer
RIFactoryVk::MT_CreateTextureSampler2D(size_t size)
{
    critical(0);
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size        = size;
    bufferInfo.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer       buffer{};
    const VkResult result = vkCreateBuffer(VulkanDevice.Device, &bufferInfo, nullptr, &buffer);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkErrorString(result));
            return nullptr;
        }

    return buffer;
}

VkImageView
RIFactoryVk::MT_CreateImageView(EImageFormat format, VkImage image, VkImageAspectFlags aspect)
{
    return VulkanDevice.CreateImageView(EImageFormat_TO_VkFormat[format], { image }, aspect, 0, 1);
}

VkSampler
RIFactoryVk::MT_CreateSampler(int minLod, int maxLod)
{
    return VulkanDevice.CreateSampler(
    VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, minLod, maxLod, VK_SAMPLER_MIPMAP_MODE_LINEAR, true, VulkanDevice.DeviceProperties.limits.maxSamplerAnisotropy);
}

void
RIFactoryVk::MT_DestroySampler(VkSampler sampler)
{
    VulkanDevice.DestroySampler(sampler);
}

void
RIFactoryVk::MT_DestroyBuffer(VkBuffer buffer, VmaAllocation allocation)
{
    VulkanDevice.DestroyBuffer(RIVulkanBuffer{ buffer, allocation });
}

void
RIFactoryVk::MT_CopyDataToMemory(VmaAllocation allocation, void* data, size_t bytes, size_t offset)
{

    void* gpuVirtualAddress = VulkanDevice.MapBuffer(RIVulkanBuffer{ nullptr, allocation });

    gpuVirtualAddress = static_cast<char*>(gpuVirtualAddress) + offset;
    memcpy(gpuVirtualAddress, data, bytes);

    VulkanDevice.UnmapBuffer(RIVulkanBuffer{ nullptr, allocation });
}

void*
RIFactoryVk::MapMemory(VmaAllocation allocation)
{
    return VulkanDevice.MapBuffer(RIVulkanBuffer{ nullptr, allocation });
}

void
RIFactoryVk::UnmapMemory(VmaAllocation allocation)
{
    VulkanDevice.UnmapBuffer(RIVulkanBuffer{ nullptr, allocation });
}

ERI_ERROR
RIFactoryVk::MT_TryAcquireNextImage(VkSwapchainKHR swapchain, uint32_t* imageIndex, VkSemaphore imageAvailable)
{

    const VkResult result = VulkanDevice.AcquireNextImage(swapchain, 1000, imageIndex, imageAvailable, nullptr);

    /*Spec: https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/vkQueueSubmit.html*/
    /*
    Return Codes

    On success, this command returns

            VK_SUCCESS

    On failure, this command returns

            VK_ERROR_OUT_OF_HOST_MEMORY

            VK_ERROR_OUT_OF_DEVICE_MEMORY

            VK_ERROR_DEVICE_LOST

            VK_TIMOUT
    */
    switch (result)
        {
            case VK_SUCCESS:
                {
                    return ERI_ERROR::SUCCESS;
                }
                break;
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                {
                    return ERI_ERROR::OUT_OF_SYSMEMORY;
                }
                break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                {
                    return ERI_ERROR::OUT_OF_GPUMEMORY;
                }
                break;
            case VK_ERROR_DEVICE_LOST:
                {
                    return ERI_ERROR::DEVICE_LOST;
                }
                break;
            case VK_TIMEOUT:
                {
                    return ERI_ERROR::ACQUIRE_TIMEOUT;
                }
        }

    return ERI_ERROR::SUCCESS;
}

std::vector<ERI_ERROR>
RIFactoryVk::MT_Present(std::vector<std::pair<VkSwapchainKHR, uint32_t>> swapchainImageIndex, std::vector<VkSemaphore> waitSemaphores)
{
    const std::vector<VkResult> result = VulkanDevice.Present(swapchainImageIndex, waitSemaphores);

    std::vector<ERI_ERROR> errors;

    std::transform(result.begin(), result.end(), std::back_inserter(errors), [](const VkResult& e) {
        switch (e)
            {
                case VK_SUCCESS:
                case VK_SUBOPTIMAL_KHR:
                    {
                        return ERI_ERROR::SUCCESS;
                    }
                    break;
                case VK_ERROR_OUT_OF_DATE_KHR:
                    {
                        return ERI_ERROR::SWAPCHAIN_OUT_OF_DATE;
                    }
                    break;
                case VK_ERROR_SURFACE_LOST_KHR:
                    {
                        return ERI_ERROR::SURFACE_LOST;
                    }
                    break;
                case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
                    {
                        return ERI_ERROR::FULLSCREEN_LOST;
                    }
                    break;
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    {
                        return ERI_ERROR::OUT_OF_SYSMEMORY;
                    }
                    break;
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    {
                        return ERI_ERROR::OUT_OF_GPUMEMORY;
                    }
                    break;
                case VK_ERROR_DEVICE_LOST:
                    {
                        return ERI_ERROR::DEVICE_LOST;
                    }
                    break;
            }

        check(0);
        return ERI_ERROR::DEVICE_LOST;
    });

    return errors;
}

RICommandPool*
RIFactoryVk::MT_CreateCommandPool2()
{
    return VulkanDevice.CreateCommandPool();
}

void
RIFactoryVk::MT_DestroyCommandPool2(RICommandPool* pool)
{
    VulkanDevice.DestroyCommandPool(pool);
}

void
RIFactoryVk::MT_SubmitQueue2(const std::vector<RICommandBuffer*>& cmds, const std::vector<VkSemaphore>& waitSemaphore, VkSemaphore finishSemaphore, RIFence* fence)
{
    VulkanDevice.SubmitToMainQueue(cmds, waitSemaphore, finishSemaphore, fence ? static_cast<RIFenceVk*>(fence)->Fence : VK_NULL_HANDLE);
}

VkCommandPool
RIFactoryVk::MT_CreateCommandPool()
{

    /* Allocate command pool*/
    VkCommandPool commandPool{};
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; /*Allow command buffers to be rerecorded individually, without this flag they all have to be reset together*/
        poolInfo.queueFamilyIndex = VulkanDevice.GetQueueFamilyIndex();

        const VkResult result = vkCreateCommandPool(VulkanDevice.Device, &poolInfo, nullptr, &commandPool);
        if (VKFAILED(result))
            {
                throw std::runtime_error(VkErrorString(result));
            }
    }

    return commandPool;
}

std::vector<VkCommandBuffer>
RIFactoryVk::MT_CreateCommandBuffers(VkCommandPool commandPool, int num)
{
    std::vector<VkCommandBuffer> commandBuffers(num);

    /*Allocate all command buffers*/
    {
        VkCommandBufferAllocateInfo info{};
        info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.pNext              = NULL;
        info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        info.commandPool        = commandPool;
        info.commandBufferCount = num;

        const VkResult result = vkAllocateCommandBuffers(VulkanDevice.Device, &info, commandBuffers.data());
        if (VKFAILED(result))
            {
                throw std::runtime_error(VkErrorString(result));
            }
    }
    return commandBuffers;
}

void
RIFactoryVk::MT_DestroyCommandPool(VkCommandPool commandPool)
{
    vkDestroyCommandPool(VulkanDevice.Device, commandPool, nullptr);
}

void
RIFactoryVk::MT_ResetCommandPool(VkCommandPool commandPool)
{
    vkResetCommandPool(VulkanDevice.Device, commandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
}

void
RIFactoryVk::MT_SubmitQueue(const std::vector<VkCommandBuffer>& commandBuffers, std::vector<VkSemaphore> waitSemaphore, VkSemaphore finishSemaphore, RIFence* fence)
{

    RIFenceVk* rifence = (RIFenceVk*)fence;

    const std::vector<VkPipelineStageFlags> waitStages(waitSemaphore.size(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

    VkSubmitInfo submitInfo{};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = (uint32_t)waitSemaphore.size();
    submitInfo.pWaitSemaphores      = waitSemaphore.data();
    submitInfo.pWaitDstStageMask    = waitStages.data();
    submitInfo.commandBufferCount   = (uint32_t)commandBuffers.size();
    submitInfo.pCommandBuffers      = commandBuffers.data();
    submitInfo.signalSemaphoreCount = finishSemaphore ? 1 : 0;
    submitInfo.pSignalSemaphores    = finishSemaphore ? &finishSemaphore : nullptr;

    const VkResult result = vkQueueSubmit(VulkanDevice.MainQueue, 1, &submitInfo, fence ? rifence->Fence : VK_NULL_HANDLE);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkErrorString(result));
        }
}

VulkanUniformBuffer*
RIFactoryVk::CreateUniformBuffer(uint32_t bytes)
{
    check(bytes > 0);
    auto ubo = VulkanDevice.CreateBufferDeviceLocalTransferBit(bytes, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    VulkanUniformBuffer vkUbo{ubo, bytes};
    _uniformBuffers.push_back(std::move(vkUbo));

    return &_uniformBuffers.back();
}

void
RIFactoryVk::DestroyUniformBuffer(VulkanUniformBuffer* ubo)
{
    std::list<VulkanUniformBuffer>::iterator it = _uniformBuffers.begin();
    while (it != _uniformBuffers.end())
        {
            if (&(*it) == ubo)
                {
                    // Iterator 'it' now points to the element
                    break;
                }
            ++it;
        }

    check(it != _uniformBuffers.end()); // was already deleted

    VulkanDevice.DestroyBuffer(ubo->Buffer);

    _uniformBuffers.erase(it);
}

void
RIFactoryVk::MT_WaitIdle()
{
    vkDeviceWaitIdle(VulkanDevice.Device);
}

VkDescriptorSetLayout
RIFactoryVk::MT_CreateDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo layoutCreateInfo)
{
    VkDescriptorSetLayout layout{};
    const VkResult        result = vkCreateDescriptorSetLayout(VulkanDevice.Device, &layoutCreateInfo, nullptr, &layout);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkErrorString(result));
        }

    return layout;
}

VkDescriptorSetLayout
RIFactoryVk::MT_CreateDescriptorSetLayout(const std::vector<RIShaderDescriptorBindings>& bindings)
{
    std::vector<VkDescriptorSetLayoutBinding> binding;

    for (const auto& b : bindings)
        {
            VkDescriptorSetLayoutBinding modelLayoutBinding{};
            modelLayoutBinding.binding = b.Binding;

            switch (b.StorageType)
                {
                    case RIShaderDescriptorBindings::Type::STATIC:
                        modelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        break;
                    case RIShaderDescriptorBindings::Type::DYNAMIC:
                        modelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                        break;
                    case RIShaderDescriptorBindings::Type::SAMPLER_COMBINED:
                        modelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        break;
                    default:
                        critical(0);
                }
            modelLayoutBinding.descriptorCount = b.Count;
            switch (b.Stage)
                {
                    case ERIShaderStage::VERTEX:
                        modelLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                        break;
                    case ERIShaderStage::FRAGMENT:
                        modelLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                        break;
                    case ERIShaderStage::ALL:
                        modelLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
                        break;
                    default:
                        check(0);
                        break;
                }
            modelLayoutBinding.pImmutableSamplers = nullptr; // Optional only for images

            binding.push_back(modelLayoutBinding);
        }

    return VulkanDevice.CreateDescriptorSetLayout(binding);
}

VkEvent
RIFactoryVk::MT_CreateEvent()
{
    VkEventCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    info.pNext = NULL;
    info.flags = 0;

    VkEvent        event{};
    const VkResult result = vkCreateEvent(VulkanDevice.Device, &info, nullptr, &event);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkErrorString(result));
        }
    return event;
}

void
RIFactoryVk::MT_DestroyEvent(VkEvent event)
{
    vkDestroyEvent(VulkanDevice.Device, event, nullptr);
}

void
RIFactoryVk::MT_ResetEvent(VkEvent event)
{
    vkResetEvent(VulkanDevice.Device, event);
}

VkPipeline
RIFactoryVk::MT_CreateWorldPipeline(VkPipelineLayout pipelineLayout,
VkRenderPass                                         renderPass,
const std::vector<VkPipelineShaderStageCreateInfo>&  shaderStages,
std::vector<RIInputSlot>                             vertexInput,
uint32_t                                             vertexStrideBytes)
{

    // Binding
    VkVertexInputBindingDescription inputBinding{};
    inputBinding.binding   = 0;
    inputBinding.stride    = vertexStrideBytes;
    inputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    // Attributes
    std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescription = RIVertexInputAttributeDescriptionBuilderVk::Create(vertexInput);
    RIVulkanPipelineBuilder                        pipe(shaderStages, { inputBinding }, vertexInputAttributeDescription, pipelineLayout, renderPass);
    pipe.AddViewport({});
    pipe.AddScissor({});
    pipe.SetDynamicState({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });

    return VulkanDevice.CreatePipeline(&pipe.PipelineCreateInfo);
}

VkPipeline
RIFactoryVk::MT_CreatePipeline(VkPipelineLayout pipelineLayout, VkRenderPass renderPass, const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages, const DRIPipelineFormat& format)
{
    // Create pipeline
    VkPipeline graphicsPipeline{};
    {
        // Binding
        VkVertexInputBindingDescription inputBinding{};
        inputBinding.binding   = 0;
        inputBinding.stride    = format.VertexStrideBytes;
        inputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        // Attributes
        std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescription = RIVertexInputAttributeDescriptionBuilderVk::Create(format.VertexInput);
        RIVulkanPipelineBuilder                        pipe(shaderStages, { inputBinding }, vertexInputAttributeDescription, pipelineLayout, renderPass);
        pipe.AddViewport({});
        pipe.AddScissor({});
        pipe.SetDynamicState({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });
        switch (format.FillMode)
            {
                case EFillMode::FILL:
                    pipe.SetPolygonMode(VK_POLYGON_MODE_FILL);
                    break;
                case EFillMode::LINE:
                    pipe.SetPolygonMode(VK_POLYGON_MODE_FILL);
                    break;
                default:
                    check(0);
                    break;
            }
        switch (format.CullMode)
            {
                case ECullMode::NONE:
                    pipe.SetCulling(VK_CULL_MODE_NONE);
                    break;
                case ECullMode::FRONT:
                    pipe.SetCulling(VK_CULL_MODE_FRONT_BIT);
                    break;
                case ECullMode::BACK:
                    pipe.SetCulling(VK_CULL_MODE_BACK_BIT);
                    break;
                default:
                    check(0);
                    break;
            }
        switch (format.DepthTestMode)
            {
                case EDepthTest::ALWAYS:
                    pipe.SetDepthTestingOp(VK_COMPARE_OP_ALWAYS);
                    break;
                case EDepthTest::NEVER:
                    pipe.SetDepthTestingOp(VK_COMPARE_OP_NEVER);
                    break;
                case EDepthTest::LESS:
                    pipe.SetDepthTestingOp(VK_COMPARE_OP_LESS);
                    break;
                case EDepthTest::LESS_OR_EQUAL:
                    pipe.SetDepthTestingOp(VK_COMPARE_OP_LESS_OR_EQUAL);
                    break;
                case EDepthTest::GREATER:
                    pipe.SetDepthTestingOp(VK_COMPARE_OP_GREATER);
                    break;
                case EDepthTest::GREATER_OR_EQUAL:
                    pipe.SetDepthTestingOp(VK_COMPARE_OP_GREATER_OR_EQUAL);
                    break;
                default:
                    pipe.SetDepthTestingOp(VK_COMPARE_OP_LESS_OR_EQUAL);
                    critical(0);
                    break;
            }
        pipe.SetDepthTesting(format.DepthTest, format.DepthWrite);
        pipe.SetDepthStencil(format.StencilTest);
        pipe.SetDepthStencilOp(VkStencilOpState{ VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_LESS_OR_EQUAL, 0, 0, 0 });

        switch (format.BlendMode)
            {
                case ERIBlendMode::Additive:
                    {
                        pipe.SetAlphaBlending();
                    }
                    break;
                case ERIBlendMode::DefaultBlendMode:
                default:
                    break;
            }

        graphicsPipeline = VulkanDevice.CreatePipeline(&pipe.PipelineCreateInfo);
    }

    return graphicsPipeline;
}

PipelineVk
RIFactoryVk::CreatePipeline(const RIShaderSource& shader, const DRIPipelineFormat& format)
{
    constexpr uint32_t MAX_SETS = 10000;

    // Convert layout to descriptor set layouts
    std::vector<VkDescriptorSetLayout> descriptorSetLayout;
    // Also gather all the descriptor pool size
    std::vector<VkDescriptorPoolSize> poolDimensions;

    // Map of descriptor set layout
    std::map<uint32_t, VkDescriptorSetLayout> setIndexToDescriptorSetLayout;

    for (const auto& set : shader.SetsLayout.SetsLayout)
        {
            std::vector<VkDescriptorSetLayoutBinding> bindings;

            for (const auto binding : set.second)
                {
                    VkDescriptorSetLayoutBinding b{};
                    b.binding            = binding.first;
                    b.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    b.descriptorCount    = binding.second.Count;
                    b.stageFlags         = VK_SHADER_STAGE_ALL_GRAPHICS;
                    b.pImmutableSamplers = nullptr;

                    switch (binding.second.StorageType)
                        {
                            case RIShaderDescriptorBindings::Type::STATIC:
                                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                                break;
                            case RIShaderDescriptorBindings::Type::DYNAMIC:
                                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                                break;
                            case RIShaderDescriptorBindings::Type::STORAGE:
                                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                                break;
                            case RIShaderDescriptorBindings::Type::SAMPLER_COMBINED:
                                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                                break;
                        }

                    switch (binding.second.Stage)
                        {
                            case ERIShaderStage::VERTEX:
                                b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                                break;
                            case ERIShaderStage::FRAGMENT:
                                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                                break;
                            case ERIShaderStage::ALL:
                                b.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
                                break;
                        }

                    // Append size
                    auto sameTypeIt = std::find_if(poolDimensions.begin(), poolDimensions.end(), [&b](const VkDescriptorPoolSize& size) { return size.type == b.descriptorType; });
                    if (sameTypeIt == poolDimensions.end())
                        {
                            VkDescriptorPoolSize poolSize{ b.descriptorType, binding.second.Count };
                            poolDimensions.push_back(poolSize);
                        }
                    else
                        {
                            sameTypeIt->descriptorCount += binding.second.Count;
                        }

                    bindings.emplace_back(std::move(b));
                }

            descriptorSetLayout.push_back(VulkanDevice.CreateDescriptorSetLayout(bindings));
            setIndexToDescriptorSetLayout[set.first] = descriptorSetLayout.back();
        }

    // Create pipeline layout from the descriptor set layouts and no push constants
    VkPipelineLayout pipelineLayout = VulkanDevice.CreatePipelineLayout(descriptorSetLayout, {});

    // Cache all descriptor sets for this given pipeline layout
    auto pipelineLayoutCacheIt = _pipelineLayoutToSetIndexDescriptorSetLayout.find(pipelineLayout);
    if (pipelineLayoutCacheIt == _pipelineLayoutToSetIndexDescriptorSetLayout.end())
        {
            _pipelineLayoutToSetIndexDescriptorSetLayout[pipelineLayout] = setIndexToDescriptorSetLayout;
        }

    auto alreadyHasDescriptorSetPool = _pipelineLayoutToDescriptorPool.back().find(pipelineLayout);
    // Create descriptor pools per frame
    if (alreadyHasDescriptorSetPool == _pipelineLayoutToDescriptorPool.back().end())
        {
            for (auto& perFrame : _pipelineLayoutToDescriptorPool)
                {
                    VkDescriptorPool pool                  = VulkanDevice.CreateDescriptorPool(poolDimensions, MAX_SETS);
                    auto             descriptorPoolManager = new RIDescriptorPoolManager(VulkanDevice.Device, pool, MAX_SETS);
                    perFrame.insert({ pipelineLayout, descriptorPoolManager });
                }
        }

    // Create render pass
    DRenderPassAttachments att; // default color format, not important, what matters is the color attachment
    att.Attachments.push_back(DRenderPassAttachment(
    VK_FORMAT_B8G8R8A8_UNORM, ERenderPassLoad::Clear, ERenderPassStore::Store, ERenderPassLayout::Undefined, ERenderPassLayout::Present, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
    const auto renderPass = MT_CreateRenderPass(att);

    // Create shader stages
    auto shaderStages = MT_CreateShaderStages(shader.SourceCode);
    // Create pipeline
    VkPipeline pipeline = MT_CreatePipeline(pipelineLayout, renderPass, shaderStages, format);

    MT_DestroyShaderStages(shaderStages);

    return PipelineVk{ pipeline, pipelineLayout };
}

VkPipeline
RIFactoryVk::MT_CreateWorldDebugPipeline(VkPipelineLayout pipelineLayout,
VkRenderPass                                              renderPass,
const std::vector<VkPipelineShaderStageCreateInfo>&       shaderStages,
std::vector<RIInputSlot>                                  vertexInput,
uint32_t                                                  vertexStrideBytes)
{
    VkVertexInputBindingDescription inputBinding{};
    inputBinding.binding   = 0;
    inputBinding.stride    = vertexStrideBytes;
    inputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    // Attributes
    std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescription = RIVertexInputAttributeDescriptionBuilderVk::Create(vertexInput);
    RIVulkanPipelineBuilder                        pipe(shaderStages, { inputBinding }, vertexInputAttributeDescription, pipelineLayout, renderPass);
    pipe.AddViewport({});
    pipe.AddScissor({});
    pipe.SetDynamicState({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_LINE_WIDTH });

    pipe.SetPolygonMode(VK_POLYGON_MODE_LINE);

    return VulkanDevice.CreatePipeline(&pipe.PipelineCreateInfo);
}

void
RIFactoryVk::MT_DestroyPipeline(VkPipeline pipeline)
{
    VulkanDevice.DestroyPipeline(pipeline);
}

RIFence*
RIFactoryVk::CreateFence(bool signaled)
{
    RIFenceVk* rifence         = new RIFenceVk(signaled);
    rifence->Device_DEPRECATED = VulkanDevice.Device;

    rifence->Fence = VulkanDevice.CreateFence(signaled);

    return (RIFence*)rifence;
}

void
RIFactoryVk::DestroyFence(RIFence* fence)
{
    check(fence->WasSignaled()); // Must be in signaled state!
    RIFenceVk* vkFence = static_cast<RIFenceVk*>(fence);
    VulkanDevice.DestroyFence(vkFence->Fence);
    delete fence;
}

VkQueryPool
RIFactoryVk::MT_CreateOcclusionQueryPool(uint32_t count)
{
    VkQueryPoolCreateInfo queryPoolInfo = {};
    queryPoolInfo.sType                 = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    queryPoolInfo.queryType             = VK_QUERY_TYPE_OCCLUSION;
    queryPoolInfo.queryCount            = count;

    VkQueryPool    queryPool{};
    const VkResult result = vkCreateQueryPool(VulkanDevice.Device, &queryPoolInfo, NULL, &queryPool);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkErrorString(result));
        }
    return queryPool;
}

void
RIFactoryVk::MT_QueryResults(VkQueryPool pool, uint32_t offset, uint32_t count, size_t bufferSize, void* pData)
{
    const VkResult result = vkGetQueryPoolResults(VulkanDevice.Device,
    pool,
    offset,
    count,
    bufferSize,
    pData,
    sizeof(uint64_t),
    // Store results a 64 bit values and wait until the results have been finished
    // If you don't want to wait, you can use VK_QUERY_RESULT_WITH_AVAILABILITY_BIT
    // which also returns the state of the result (ready) in the result
    VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

    if (VKFAILED(result))
        {
            throw std::runtime_error(VkErrorString(result));
        }
}

void
RIFactoryVk::MT_DestroyQueryPool(VkQueryPool pool)
{
    vkDestroyQueryPool(VulkanDevice.Device, pool, nullptr);
}

} // namespace Fox

#endif