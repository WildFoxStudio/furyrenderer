
#include "VulkanContext.h"

#include "ResourceTransfer.h"
#include "UtilsVK.h"

#include <algorithm>
#include <limits>
#include <string>

namespace Fox
{

VulkanContext::VulkanContext(const DContextConfig* const config) : _warningOutput(config->warningFunction), _logOutput(config->logOutputFunction)
{
    _initializeVolk();
    _initializeInstance();
    _initializeDebugger();
    _initializeVersion();
    _initializeDevice();
    _initializeStagingBuffer(config->stagingBufferSize);
    // Command pools per frame
    {
        std::generate_n(std::back_inserter(_cmdPool), NUM_OF_FRAMES_IN_FLIGHT, [this]() { return RICommandPoolManager(Device.CreateCommandPool()); });
    }
    // Initialize per frame pipeline layout map to descriptor pool manager
    _pipelineLayoutToDescriptorPool.resize(NUM_OF_FRAMES_IN_FLIGHT);

    // Fences per frame
    {
        constexpr bool fenceSignaled = true; // since the loop begins waiting on a fence the fence must be already signaled otherwise it will timeout
        std::generate_n(std::back_inserter(_fence), NUM_OF_FRAMES_IN_FLIGHT, [this, fenceSignaled = fenceSignaled]() { return Device.CreateFence(fenceSignaled); });
    }

    // Semaphores per frame
    {
        std::generate_n(std::back_inserter(_workFinishedSemaphores), NUM_OF_FRAMES_IN_FLIGHT, [this]() { return Device.CreateVkSemaphore(); });
    }
}

void
VulkanContext::_initializeVolk()
{
    Log("Initializing VOLK");
    const VkResult result = volkInitialize();
    if (result != VK_SUCCESS)
        {
            Log("Failed to initialize volk");
            throw std::runtime_error("Failed to initialize volk");
        }
}

void
VulkanContext::_initializeInstance()
{
    // Initialize instance
    auto validValidationLayers = _getInstanceSupportedValidationLayers(_validationLayers);
    auto validExtensions       = _getInstanceSupportedExtensions(_instanceExtensionNames);

    const auto result = Instance.Init("Application", validValidationLayers, validExtensions);
    if (VKFAILED(result))
        {
            throw std::runtime_error("Could not create a vulkan instance" + std::string(VkUtils::VkErrorString(result)));
        }
}

VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanContext::_vulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
VkDebugUtilsMessageTypeFlagsEXT                                            messageType,
const VkDebugUtilsMessengerCallbackDataEXT*                                pCallbackData,
void*                                                                      pUserData)
{
    VulkanContext* context = static_cast<VulkanContext*>(pUserData);
    check(context);

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            context->Warning("Validation layer[WARNING]: " + std::string(pCallbackData->pMessage));
        }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            context->Warning("Validation layer[ERROR]: " + std::string(pCallbackData->pMessage));
        }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        {
            context->Warning("Validation layer[VERBOSE]: " + std::string(pCallbackData->pMessage));
        }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        {
            context->Log("Validation layer[INFO]: " + std::string(pCallbackData->pMessage));
        }

    // check(false);
    return VK_FALSE;
}

void
VulkanContext::_initializeDebugger()
{
#ifdef _DEBUG
    const VkResult result = Instance.CreateDebugUtilsMessenger(VulkanContext::_vulkanDebugCallback, this);
    if (VKFAILED(result))
        {
            Warning("Failed to create vulkan debug utils messenger");
        }
#endif
}

void
VulkanContext::_initializeVersion()
{
    auto FN_vkEnumerateInstanceVersion = PFN_vkEnumerateInstanceVersion(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));
    if (vkEnumerateInstanceVersion == nullptr)
        {
            Warning("Failed to vkGetInstanceProcAddr for vkEnumerateInstanceVersion");
        }
    else
        {
            uint32_t       instanceVersion = VK_API_VERSION_1_0;
            const VkResult result          = vkEnumerateInstanceVersion(&instanceVersion);
            if (VKFAILED(result))
                {
                    const std::string out = "Failed to vkEnumerateInstanceVersion because:" + std::string(VkUtils::VkErrorString(result));
                    Warning(out);
                }
            else
                {
                    // Extract version info
                    uint32_t major, minor, patch;
                    major = VK_API_VERSION_MAJOR(instanceVersion);
                    minor = VK_API_VERSION_MINOR(instanceVersion);
                    patch = VK_API_VERSION_PATCH(instanceVersion);

                    const std::string out = "Vulkan version:" + std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
                    Log(out);
                }
        }
}

void
VulkanContext::_initializeDevice()
{
    // Query physical device
    VkPhysicalDevice physicalDevice = _queryBestPhysicalDevice();

    // Check validation layers and extensions support for the device
    auto validDeviceValidationLayers = _getDeviceSupportedValidationLayers(physicalDevice, _validationLayers);
    auto validDeviceExtensions       = _getDeviceSupportedExtensions(physicalDevice, _deviceExtensionNames);

    // Create device
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy                      = VK_TRUE;
    deviceFeatures.shaderSampledImageArrayDynamicIndexing = VK_TRUE;
    deviceFeatures.fillModeNonSolid                       = VK_TRUE;

    const auto result = Device.Create(Instance, physicalDevice, _deviceExtensionNames, deviceFeatures, _validationLayers);
    if (VKFAILED(result))
        {
            throw std::runtime_error("Could not create a vulkan device" + std::string(VkUtils::VkErrorString(result)));
        }

    // replacing global function pointers with functions retrieved with vkGetDeviceProcAddr
    volkLoadDevice(Device.Device);
}

void
VulkanContext::_initializeStagingBuffer(uint32_t stagingBufferSize)
{
    // Create staging buffer with manager
    _stagingBuffer = Device.CreateBufferHostVisible(stagingBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    std::generate_n(std::back_inserter(_perFrameCopySizes), NUM_OF_FRAMES_IN_FLIGHT, []() { return std::vector<uint32_t>(); });
    unsigned char* _stagingBufferPtr = (unsigned char*)Device.MapBuffer(_stagingBuffer);
    _stagingBufferManager            = std::make_unique<RingBufferManager>(stagingBufferSize, _stagingBufferPtr);
}

void
VulkanContext::_deinitializeStagingBuffer()
{ // Unmap and destroy staging buffer
    _stagingBufferManager.reset();
    Device.UnmapBuffer(_stagingBuffer);
    Device.DestroyBuffer(_stagingBuffer);
}

VulkanContext::~VulkanContext()
{
    check(_swapchains.size() == 0);
    vkDeviceWaitIdle(Device.Device);

    // Wait for all fences to signal
    std::for_each(_fence.begin(), _fence.end(), [this](VkFence fence) {
        const VkResult result = vkWaitForFences(Device.Device, 1, &fence, VK_TRUE /*Wait all*/, MAX_FENCE_TIMEOUT);
        if (VKFAILED(result))
            {
                throw std::runtime_error(VkUtils::VkErrorString(result));
            }
    });
    _fence.clear();

    std::for_each(_workFinishedSemaphores.begin(), _workFinishedSemaphores.end(), [this](VkSemaphore semaphore) { Device.DestroyVkSemaphore(semaphore); });
    _workFinishedSemaphores.clear();

    // Free command pools
    std::for_each(_cmdPool.begin(), _cmdPool.end(), [this](const RICommandPoolManager& pool) { Device.DestroyCommandPool(pool.GetCommandPool()); });
    _cmdPool.clear();

    // Destroy fences
    std::for_each(_fence.begin(), _fence.end(), [this](VkFence fence) { Device.DestroyFence(fence); });
    _fence.clear();

    // Free pipeline layouts and their descriptor set layouts used to create them
    for (const auto& pair : _pipelineLayoutToSetIndexDescriptorSetLayout)
        {
            Device.DestroyPipelineLayout(pair.first);

            for (const auto& p : pair.second)
                {
                    Device.DestroyDescriptorSetLayout(p.second);
                }
        }
    _pipelineLayoutToSetIndexDescriptorSetLayout.clear();

    // Destroy all descriptor pools managers
    _pipelineLayoutToDescriptorPool.clear();

    _deinitializeStagingBuffer();

    FlushDeletedBuffers();

    for (const auto renderPass : _renderPasses)
        {
            Device.DestroyRenderPass(renderPass);
        }
    _renderPasses.clear();

    Device.Deinit();
    Instance.Deinit();
}

bool
VulkanContext::CreateSwapchain(const WindowData* windowData, EPresentMode& presentMode, EFormat& outFormat, DSwapchain* swapchain)
{
    VkSurfaceKHR surface{};
    {
        const auto result = Instance.CreateSurfaceFromWindow(*windowData, &surface);
        if (VKFAILED(result))
            {
                throw std::runtime_error("Failed to create surface from window " + std::string(VkUtils::VkErrorString(result)));
            }
    }

    const auto formats           = Device.GetSurfaceFormats(surface);
    outFormat                    = VkUtils::convertVkFormat(formats.front().format);
    const auto validPresentModes = Device.GetSurfacePresentModes(surface);

    auto       vkPresentMode = VkUtils::convertPresentMode(presentMode);
    const bool validPresentMode =
    std::find_if(validPresentModes.cbegin(), validPresentModes.cend(), [vkPresentMode](const VkPresentModeKHR& mode) { return mode == vkPresentMode; }) != validPresentModes.cend();
    if (!validPresentMode)
        {
            vkPresentMode = validPresentModes.front();
        }

    const auto capabilities = Device.GetSurfaceCapabilities(surface);

    VkSwapchainKHR vkSwapchain{};
    {
        const auto result = Device.CreateSwapchainFromSurface(surface, formats.at(0), vkPresentMode, capabilities, &vkSwapchain);
        if (VKFAILED(result))
            {
                throw std::runtime_error("Failed to create swapchain " + std::string(VkUtils::VkErrorString(result)));
            }
    }

    DSwapchainVulkan swapchainVulkan{};
    swapchainVulkan.Surface      = surface;
    swapchainVulkan.Capabilities = capabilities;
    swapchainVulkan.Format       = formats.at(0);
    swapchainVulkan.PresentMode  = vkPresentMode;
    swapchainVulkan.Swapchain    = vkSwapchain;
    swapchainVulkan.Images       = Device.GetSwapchainImages(vkSwapchain);

    std::transform(swapchainVulkan.Images.begin(), swapchainVulkan.Images.end(), std::back_inserter(swapchainVulkan.ImageViews), [this, format = swapchainVulkan.Format.format](const VkImage& image) {
        VkImageView imageView{};
        const auto  result = Device.CreateImageView(format, image, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, &imageView);
        if (VKFAILED(result))
            {
                throw std::runtime_error("Failed to create swapchain imageView " + std::string(VkUtils::VkErrorString(result)));
            }
        return imageView;
    });

    std::generate_n(std::back_inserter(swapchainVulkan.ImageAvailableSemaphore), NUM_OF_FRAMES_IN_FLIGHT, [this]() { return Device.CreateVkSemaphore(); });

    DRenderPassAttachments attachments;
    DRenderPassAttachment  color(VkUtils::convertVkFormat(swapchainVulkan.Format.format),
    ESampleBit::COUNT_1_BIT,
    ERenderPassLoad::Clear,
    ERenderPassStore::Store,
    ERenderPassLayout::Undefined,
    ERenderPassLayout::Present,
    EAttachmentReference::COLOR_ATTACHMENT);
    attachments.Attachments.emplace_back(color);

    VkRenderPass renderPass = _createRenderPass(attachments);

    DFramebufferVulkan vulkanFramebuffer;
    std::transform(swapchainVulkan.ImageViews.begin(), swapchainVulkan.ImageViews.end(), std::back_inserter(vulkanFramebuffer.Framebuffers), [this, &capabilities, renderPass](VkImageView view) {
        return Device.CreateFramebuffer({ view }, capabilities.currentExtent.width, capabilities.currentExtent.height, renderPass);
    });

    _framebuffers.emplace_back(std::move(vulkanFramebuffer));
    swapchainVulkan.Framebuffers = &_framebuffers.back();

    _swapchains.emplace_back(std::move(swapchainVulkan));

    *swapchain = &_swapchains.back();
    return true;
}

void
VulkanContext::DestroySwapchain(const DSwapchain swapchain)
{
    DSwapchainVulkan* vkSwapchain = static_cast<DSwapchainVulkan*>(swapchain);

    DestroyFramebuffer(vkSwapchain->Framebuffers);

    std::for_each(vkSwapchain->ImageViews.begin(), vkSwapchain->ImageViews.end(), [this](const VkImageView& imageView) { Device.DestroyImageView(imageView); });
    vkSwapchain->ImageViews.clear();

    std::for_each(vkSwapchain->ImageAvailableSemaphore.begin(), vkSwapchain->ImageAvailableSemaphore.end(), [this](const VkSemaphore& semaphore) { Device.DestroyVkSemaphore(semaphore); });
    vkSwapchain->ImageAvailableSemaphore.clear();

    Device.DestroySwapchain(vkSwapchain->Swapchain);
    Instance.DestroySurface(vkSwapchain->Surface);

    _swapchains.erase(std::find_if(_swapchains.begin(), _swapchains.end(), [swapchain](const DSwapchainVulkan& s) { return &s == swapchain; }));
}

DFramebuffer
VulkanContext::CreateSwapchainFramebuffer(DSwapchain swapchain)
{
    return static_cast<DSwapchainVulkan*>(swapchain)->Framebuffers;
}

void
VulkanContext::DestroyFramebuffer(DFramebuffer framebuffer)
{
    DFramebufferVulkan* fboVulkan = static_cast<DFramebufferVulkan*>(framebuffer);
    _deferDestruction([this, fboVulkan]() {
        std::for_each(fboVulkan->Framebuffers.begin(), fboVulkan->Framebuffers.end(), [this](const VkFramebuffer& framebuffer) { Device.DestroyFramebuffer(framebuffer); });
        fboVulkan->Framebuffers.clear();

        _framebuffers.erase(std::find_if(_framebuffers.begin(), _framebuffers.end(), [fboVulkan](const DFramebufferVulkan& s) { return &s == fboVulkan; }));
    });
}

DBuffer
VulkanContext::CreateVertexBuffer(uint32_t size)
{
    DBufferVulkan buf{ EBufferType::VERTEX_INDEX_BUFFER, size };

    buf.Buffer = Device.CreateBufferDeviceLocalTransferBit(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    _vertexBuffers.emplace_back(std::move(buf));

    return &_vertexBuffers.back();
}

void
VulkanContext::DestroyVertexBuffer(DBuffer buffer)
{
    check(buffer->Type == EBufferType::VERTEX_INDEX_BUFFER);
    DBufferVulkan* bufferVulkan = static_cast<DBufferVulkan*>(buffer);

    _deferDestruction([this, buffer, bufferVulkan]() {
        Device.DestroyBuffer(bufferVulkan->Buffer);
        _vertexBuffers.erase(std::find_if(_vertexBuffers.begin(), _vertexBuffers.end(), [buffer](const DBufferVulkan& b) { return &b == buffer; }));
    });
}

DBuffer
VulkanContext::CreateUniformBuffer(uint32_t size)
{
    check(size <= 16000); // MAX 16KB limitation
    DBufferVulkan buf{ EBufferType::UNIFORM_BUFFER_OBJECT, size };

    buf.Buffer = Device.CreateBufferDeviceLocalTransferBit(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    _uniformBuffers.emplace_back(std::move(buf));

    return &_uniformBuffers.back();
}

void
VulkanContext::DestroyUniformBuffer(DBuffer buffer)
{
    check(buffer->Type == EBufferType::UNIFORM_BUFFER_OBJECT);
    DBufferVulkan* bufferVulkan = static_cast<DBufferVulkan*>(buffer);

    _deferDestruction([this, buffer, bufferVulkan]() {
        Device.DestroyBuffer(bufferVulkan->Buffer);
        _uniformBuffers.erase(std::find_if(_uniformBuffers.begin(), _uniformBuffers.end(), [buffer](const DBufferVulkan& b) { return &b == buffer; }));
    });
}

DImage
VulkanContext::CreateImage(EFormat format, uint32_t width, uint32_t height, uint32_t mipMapCount)
{
    DImageVulkan image;

    const auto vkFormat = VkUtils::convertFormat(format);

    image.Image = Device.CreateImageDeviceLocal(
    width, height, mipMapCount, vkFormat, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    const VkResult result = Device.CreateImageView(vkFormat, image.Image.Image, VK_IMAGE_ASPECT_COLOR_BIT, 0, mipMapCount, &image.View);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkUtils::VkErrorString(result));
        }

    // Create default sampler
    image.Sampler = Device.CreateSampler(VK_FILTER_NEAREST, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, 0, mipMapCount, VK_SAMPLER_MIPMAP_MODE_NEAREST, true, 16);

    _images.emplace_back(std::move(image));

    return &_images.back();
}

void
VulkanContext::DestroyImage(DImage image)
{
    DImageVulkan* const imageVulkan = static_cast<DImageVulkan*>(image);

    _deferDestruction([this, image, imageVulkan]() {
        Device.DestroyImageView(imageVulkan->View);
        Device.DestroyImage(imageVulkan->Image);
        Device.DestroySampler(imageVulkan->Sampler);

        _images.erase(std::find_if(_images.begin(), _images.end(), [imageVulkan](const DImageVulkan& b) { return &b == imageVulkan; }));
    });
}

DVertexInputLayout
VulkanContext::CreateVertexLayout(const std::vector<VertexLayoutInfo>& info)
{
    DVertexInputLayoutVulkan input;
    input.VertexInputAttributes.reserve(info.size());

    unsigned int location = 0;
    for (const auto& i : info)
        {
            VkVertexInputAttributeDescription attr{};
            attr.location = location++; // uint32_t
            attr.binding  = 0; // uint32_t
            attr.format   = VkUtils::convertFormat(i.Format); // VkFormat
            attr.offset   = i.ByteOffset; // uint32_t

            // Emplace
            input.VertexInputAttributes.emplace_back(std::move(attr));
        }
    // Not vulkan pointers so no need to delete, just a POD struct
    _vertexLayouts.emplace_back(std::move(input));
    return &_vertexLayouts.back();
}

DPipeline
VulkanContext::CreatePipeline(const ShaderSource& shader, const PipelineFormat& format)
{
    constexpr uint32_t MAX_SETS_PER_POOL = 100; // Temporary, should not have a limit pool of pools

    std::vector<VkDescriptorSetLayout>        descriptorSetLayout;
    std::map<uint32_t, VkDescriptorSetLayout> setIndexToSetLayout;
    for (const auto& setPair : shader.SetsLayout.SetsLayout)
        {
            const auto descriptorSetBindings = VkUtils::convertDescriptorBindings(setPair.second);
            descriptorSetLayout.push_back(Device.CreateDescriptorSetLayout(descriptorSetBindings));
            setIndexToSetLayout[setPair.first] = descriptorSetLayout.back();
        }
    // Can return already cached pipeline layout if exists
    VkPipelineLayout pipelineLayout = Device.CreatePipelineLayout(descriptorSetLayout, {});

    // Cache all descriptor sets layouts for this given pipeline layout
    auto pipelineLayoutCacheIt = _pipelineLayoutToSetIndexDescriptorSetLayout.find(pipelineLayout);
    if (pipelineLayoutCacheIt == _pipelineLayoutToSetIndexDescriptorSetLayout.end())
        {
            _pipelineLayoutToSetIndexDescriptorSetLayout[pipelineLayout] = setIndexToSetLayout;

            // Create per frame descriptor pool for this pipeline layout
            const auto poolDimensions = VkUtils::computeDescriptorSetsPoolSize(shader.SetsLayout.SetsLayout);
            for (auto i = 0; i < NUM_OF_FRAMES_IN_FLIGHT; i++)
                {
                    auto poolManager = std::make_shared<RIDescriptorPoolManager>(Device.Device, poolDimensions, MAX_SETS_PER_POOL);
                    auto pair        = std::make_pair(pipelineLayout, std::move(poolManager));
                    _pipelineLayoutToDescriptorPool[i].emplace(std::move(pair));
                }
        }

    // Create the shaders
    std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfo;
    {
        {
            VkShaderModule vertex{};
            const VkResult result = VkUtils::createShaderModule(Device.Device, shader.SourceCode.VertexShader, &vertex);
            if (VKFAILED(result))
                {
                    throw std::runtime_error(VkUtils::VkErrorString(result));
                }
            shaderStageCreateInfo.push_back(VkUtils::createShaderStageInfo(VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, vertex));
        }
        {
            VkShaderModule pixel{};
            const VkResult result = VkUtils::createShaderModule(Device.Device, shader.SourceCode.PixelShader, &pixel);
            if (VKFAILED(result))
                {
                    throw std::runtime_error(VkUtils::VkErrorString(result));
                }
            shaderStageCreateInfo.push_back(VkUtils::createShaderStageInfo(VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, pixel));
        }
    }

    // Create render pass
    DRenderPassAttachments att; // default color format, not important, what matters is the color attachment
    att.Attachments.push_back(DRenderPassAttachment(EFormat::R8G8B8A8_UNORM,
    ESampleBit::COUNT_1_BIT,
    ERenderPassLoad::Clear,
    ERenderPassStore::Store,
    ERenderPassLayout::Undefined,
    ERenderPassLayout::Present,
    EAttachmentReference::COLOR_ATTACHMENT));
    const auto renderPass = _createRenderPass(att);

    DPipelineVulkan pipeline{};
    pipeline.Pipeline       = _createPipeline(pipelineLayout, renderPass, shaderStageCreateInfo, format);
    pipeline.PipelineLayout = pipelineLayout;

    // Free shader modules
    for (const auto& stage : shaderStageCreateInfo)
        {
            vkDestroyShaderModule(Device.Device, stage.module, nullptr);
        }

    _pipelines.emplace_back(std::move(pipeline));
    return &_pipelines.back();
}

void
VulkanContext::DestroyPipeline(DPipeline pipeline)
{
    // We destroy only the pipeline, the pipeline layouts are destroyed with the device
    DPipelineVulkan* pipelineVulkan = static_cast<DPipelineVulkan*>(pipeline);

    _deferDestruction([this, pipelineVulkan]() {
        Device.DestroyPipeline(pipelineVulkan->Pipeline);

        _pipelines.erase(std::find_if(_pipelines.begin(), _pipelines.end(), [pipelineVulkan](const DPipelineVulkan& b) { return &b == pipelineVulkan; }));
    });
}

void
VulkanContext::_deferDestruction(DeleteFn&& fn)
{
    const auto index               = _frameIndex + NUM_OF_FRAMES_IN_FLIGHT;
    auto       foundSameFrameIndex = std::find_if(_deletionQueue.begin(), _deletionQueue.end(), [index](const FramesWaitToDeletionList& pair) { return pair.first == index; });

    if (foundSameFrameIndex == _deletionQueue.end())
        {
            std::vector<DeleteFn> lst{ std::move(fn) };
            auto                  pair = std::make_pair(index, std::move(lst));
            _deletionQueue.emplace_back(std::move(pair));
        }
    else
        {
            foundSameFrameIndex->second.emplace_back(std::move(fn));
        }
}

VkPipeline
VulkanContext::_createPipeline(VkPipelineLayout pipelineLayout, VkRenderPass renderPass, const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages, const PipelineFormat& format)
{
    // Create pipeline
    VkPipeline graphicsPipeline{};
    {
        // Binding
        check(format.VertexInput != nullptr);
        const auto vertexInputAttributes = static_cast<DVertexInputLayoutVulkan*>(format.VertexInput)->VertexInputAttributes;

        VkVertexInputBindingDescription inputBinding{};
        inputBinding.binding   = 0;
        inputBinding.stride    = format.VertexStrideBytes;
        inputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        // Attributes
        RIVulkanPipelineBuilder pipe(shaderStages, { inputBinding }, vertexInputAttributes, pipelineLayout, renderPass);
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

        graphicsPipeline = Device.CreatePipeline(&pipe.PipelineCreateInfo);
    }

    return graphicsPipeline;
}

void
VulkanContext::SubmitPass(RenderPassData&& data)
{
    _drawCommands.emplace_back(std::move(data));
}

void
VulkanContext::SubmitCopy(CopyDataCommand&& data)
{
    _transferCommands.emplace_back(std::move(data));
}

void
VulkanContext::AdvanceFrame()
{
    // Wait for previous work to complete wait fence to signal
    const VkResult result = vkWaitForFences(Device.Device, 1, &_fence[_frameIndex], VK_TRUE /*Wait all*/, MAX_FENCE_TIMEOUT);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkUtils::VkErrorString(result));
        }

    // Reset command pool
    auto& commandPool = _cmdPool[_frameIndex];
    commandPool.Reset();

    _performDeletionQueue();

    _performCopyOperations();

    // Acquire swapchain images
    // Recreate swapchain if necessary
    std::vector<VkSemaphore> imageAvailableSemaphores;
    imageAvailableSemaphores.reserve(_swapchains.size());
    std::vector<std::pair<VkSwapchainKHR, uint32_t>> swapchainImageIndex;
    swapchainImageIndex.reserve(_swapchains.size());
    {
        for (auto& swapchain : _swapchains)
            {
                const VkResult result = Device.AcquireNextImage(swapchain.Swapchain, UINT64_MAX, &swapchain.CurrentImageIndex, swapchain.ImageAvailableSemaphore[_frameIndex], VK_NULL_HANDLE);
                /*Spec: https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/vkQueueSubmit.html*/
                /*
                Return Codes

                On success, this command returns
                        VK_SUCCESS

                        VK_TIMEOUT

                        VK_NOT_READY

                        VK_SUBOPTIMAL_KHR

                On failure, this command returns
                        VK_ERROR_OUT_OF_HOST_MEMORY

                        VK_ERROR_OUT_OF_DEVICE_MEMORY

                        VK_ERROR_DEVICE_LOST

                        VK_ERROR_OUT_OF_DATE_KHR

                        VK_ERROR_SURFACE_LOST_KHR

                        VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT
                */
                switch (result)
                    {
                        case VK_SUCCESS:
                        case VK_NOT_READY:
                            break;
                        case VK_SUBOPTIMAL_KHR:
                        case VK_ERROR_OUT_OF_DATE_KHR:
                            {
                                _recreateSwapchainBlocking(&swapchain);
                                const VkResult lastResult =
                                Device.AcquireNextImage(swapchain.Swapchain, UINT64_MAX, &swapchain.CurrentImageIndex, swapchain.ImageAvailableSemaphore[_frameIndex], VK_NULL_HANDLE);
                                if (VKFAILED(lastResult))
                                    {
                                        throw std::runtime_error(VkUtils::VkErrorString(result));
                                    }
                            }
                            break;
                        case VK_TIMEOUT:
                        default:
                            throw std::runtime_error(VkUtils::VkErrorString(result));
                            break;
                    }

                imageAvailableSemaphores.push_back(swapchain.ImageAvailableSemaphore[_frameIndex]);
                swapchainImageIndex.push_back(std::make_pair(swapchain.Swapchain, swapchain.CurrentImageIndex));
            }
    }

    // record drawing commands
    {
        for (const auto& pass : _drawCommands)
            {
                auto cmd = commandPool.Allocate()->Cmd;

                VkCommandBufferBeginInfo beginInfo{};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

                vkBeginCommandBuffer(cmd, &beginInfo);
                {
                    // For each render pass bind all necessary
                    {
                        VkRenderPassBeginInfo renderPassInfo{};
                        renderPassInfo.sType      = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                        renderPassInfo.renderPass = _createRenderPass(pass.RenderPass);

                        DFramebufferVulkan* framebufferVulkan = static_cast<DFramebufferVulkan*>(pass.Framebuffer);
                        {
                            // Bind framebuffer or swapchain framebuffer based on the current image index
                            if (framebufferVulkan->Framebuffers.size() == 1)
                                {
                                    renderPassInfo.framebuffer = framebufferVulkan->Framebuffers.front();
                                }
                            else
                                {
                                    auto swapchain = std::find_if(
                                    _swapchains.begin(), _swapchains.end(), [framebufferVulkan](const DSwapchainVulkan& swapchain) { return swapchain.Framebuffers == framebufferVulkan; });
                                    check(swapchain != _swapchains.end());
                                    renderPassInfo.framebuffer = framebufferVulkan->Framebuffers[swapchain->CurrentImageIndex];
                                }
                        }

                        renderPassInfo.renderArea.offset = { 0, 0 };
                        renderPassInfo.renderArea.extent = { (uint32_t)pass.Viewport.w, (uint32_t)pass.Viewport.h };
                        // Convert clear values to vkClearValues
                        std::vector<VkClearValue> vkClearValues;
                        std::transform(pass.ClearValues.begin(), pass.ClearValues.end(), std::back_inserter(vkClearValues), [](const DClearValue& v) {
                            VkClearValue value;
                            //@TODO Risky copy of union, find a better way to handle this
                            memcpy(value.color.float32, v.color.float32, sizeof(VkClearValue));
                            return value;
                        });

                        renderPassInfo.clearValueCount = (uint32_t)vkClearValues.size();
                        renderPassInfo.pClearValues    = vkClearValues.data();
                        vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                    }
                    {
                        VkViewport viewport{ pass.Viewport.x, pass.Viewport.y, pass.Viewport.w, pass.Viewport.h, pass.Viewport.znear, pass.Viewport.zfar };
                        vkCmdSetViewport(cmd, 0, 1, &viewport);
                        VkRect2D scissor{ 0, 0, pass.Viewport.w, pass.Viewport.h };
                        vkCmdSetScissor(cmd, 0, 1, &scissor);
                    }
                    // for each draw command
                    {
                        for (const auto& draw : pass.DrawCommands)
                            {
                                DPipelineVulkan* pipelineVulkan = static_cast<DPipelineVulkan*>(draw.Pipeline);
                                // Bind pipeline
                                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineVulkan->Pipeline);
                                // find descriptor set from cache and bind them
                                auto descriptorPoolManager = _pipelineLayoutToDescriptorPool[_frameIndex].find(pipelineVulkan->PipelineLayout)->second;

                                // For each set to bind
                                for (const auto& set : draw.DescriptorSetBindings)
                                    {
                                        // Get descriptor set layout for this set
                                        auto descriptorSetLayout = _pipelineLayoutToSetIndexDescriptorSetLayout.find(pipelineVulkan->PipelineLayout)->second[set.first];

                                        auto binder = descriptorPoolManager->CreateDescriptorSetBinder();
                                        for (const auto& bindingPair : set.second)
                                            {
                                                const auto  bindingIndex = bindingPair.first;
                                                const auto& binding      = bindingPair.second;
                                                switch (binding.Type)
                                                    {
                                                        case EBindingType::UNIFORM_BUFFER_OBJECT:
                                                            {
                                                                check(binding.Buffers.size() == 1);
                                                                const auto&    b      = binding.Buffers.front();
                                                                DBufferVulkan* buffer = static_cast<DBufferVulkan*>(b);
                                                                binder.BindUniformBuffer(bindingIndex, buffer->Buffer.Buffer, 0, VK_WHOLE_SIZE);
                                                            }
                                                            break;
                                                        case EBindingType::SAMPLER:
                                                            {
                                                                std::vector<std::pair<VkImageView, VkSampler>> pair;

                                                                std::transform(binding.Images.begin(), binding.Images.end(), std::back_inserter(pair), [](const DImage i) {
                                                                    DImageVulkan* image = static_cast<DImageVulkan*>(i);
                                                                    return std::make_pair(image->View, image->Sampler);
                                                                });

                                                                binder.BindCombinedImageSamplerArray(bindingIndex, pair);
                                                            }
                                                            break;
                                                        default:
                                                            check(0);
                                                            break;
                                                    }
                                            }

                                        binder.BindDescriptorSet(cmd, pipelineVulkan->PipelineLayout, descriptorSetLayout, set.first);
                                    }

                                // draw call
                                DBufferVulkan* vertexBuffer = static_cast<DBufferVulkan*>(draw.VertexBuffer);
                                VkDeviceSize   offset{ 0 };
                                vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer->Buffer.Buffer, &offset);
                                vkCmdDraw(cmd, draw.VerticesCount, 1, draw.BeginVertex, 0);
                            }
                    }

                    // end renderpass
                    vkCmdEndRenderPass(cmd);
                }

                vkEndCommandBuffer(cmd);
            }

        // Clear draw commands vector
        _drawCommands.clear();
    }

    _submitCommands();

    // Present
    {
        auto recordedCommandBuffers = commandPool.GetRecorded();

        if (recordedCommandBuffers.size() > 0 && _swapchains.size() > 0)
            {
                std::vector<VkResult> presentError = Device.Present(swapchainImageIndex, { _workFinishedSemaphores[_frameIndex] });
                for (size_t i = 0; i < presentError.size(); i++)
                    {
                        if (presentError[i] != VK_SUCCESS)
                            {
                                const VkSwapchainKHR swapchainInErrorState = swapchainImageIndex[i].first;
                                auto                 found =
                                std::find_if(_swapchains.begin(), _swapchains.end(), [swapchainInErrorState](DSwapchainVulkan swapchain) { return swapchain.Swapchain == swapchainInErrorState; });

                                _recreateSwapchainBlocking(&(*found));
                                // throw std::runtime_error(VkErrorString(presentError[i]));
                            }
                    }
            }
    }

    // Increment the frame index
    _frameIndex = (_frameIndex + 1) % NUM_OF_FRAMES_IN_FLIGHT;
}

void
VulkanContext::FlushDeletedBuffers()
{
    if (_deletionQueue.size() > 0)
        {
            WaitDeviceIdle();
            while (_deletionQueue.size() > 0)
                {
                    _performDeletionQueue();
                }
        }
}

unsigned char*
VulkanContext::GetAdapterDescription() const
{
    return nullptr;
}

size_t
VulkanContext::GetAdapterDedicatedVideoMemory() const
{
    return 0;
}

void
VulkanContext::Warning(const std::string& error)
{
    if (_warningOutput)
        {
            _warningOutput(error.data());
        }
}

void
VulkanContext::Log(const std::string& error)
{
    if (_logOutput)
        {
            _logOutput(error.data());
        }
}

void
VulkanContext::_performCopyOperations()
{
    // Reset command pool
    auto& commandPool = _cmdPool[_frameIndex];

    // Perform copy commands
    {
        // Sort from smallest to largest data
        std::sort(_transferCommands.begin(), _transferCommands.end(), [](const CopyDataCommand& a, const CopyDataCommand& b) {
            uint32_t aSize{};
            if (a.UniformCommand.has_value())
                {
                    aSize = a.UniformCommand.value().Data.size();
                }
            if (a.VertexCommand.has_value())
                {
                    aSize = a.VertexCommand.value().Data.size();
                }
            uint32_t bSize{};
            if (b.UniformCommand.has_value())
                {
                    bSize = b.UniformCommand.value().Data.size();
                }
            if (a.VertexCommand.has_value())
                {
                    bSize = b.VertexCommand.value().Data.size();
                }

            return aSize < bSize;
        });

        // Pop previous frame allocation, freeing space
        {
            for (const uint32_t bytes : _perFrameCopySizes[_frameIndex])
                {
                    _stagingBufferManager->Pop(bytes);
                }
            _perFrameCopySizes[_frameIndex].clear();
        }

        // Copy
        CResourceTransfer transfer(commandPool.Allocate()->Cmd);
        auto              it = _transferCommands.begin();
        for (; it != _transferCommands.end(); it++)
            {

                if (it->VertexCommand.has_value() || it->UniformCommand.has_value())
                    {
                        DBufferVulkan*                    destination{};
                        const std::vector<unsigned char>* data{};
                        uint32_t*                         offset{};
                        if (it->VertexCommand.has_value())
                            {
                                destination = static_cast<DBufferVulkan*>(it->VertexCommand.value().Destination);
                                data        = &it->VertexCommand.value().Data;
                                offset      = &it->VertexCommand.value().DestOffset;
                            }
                        if (it->UniformCommand.has_value())
                            {
                                destination = static_cast<DBufferVulkan*>(it->UniformCommand.value().Destination);
                                data        = &it->UniformCommand.value().Data;
                                offset      = &it->UniformCommand.value().DestOffset;
                            }
                        check(destination != nullptr);
                        check(data != nullptr);
                        check(offset != nullptr);

                        // If not space left in staging buffer stop copying
                        const auto capacity = _stagingBufferManager->Capacity();
                        if (capacity < data->size())
                            {
                                const auto freeSpace = _stagingBufferManager->MaxSize - _stagingBufferManager->Size();
                                if (freeSpace - capacity < data->size())
                                    {
                                        break;
                                    }
                                else
                                    {
                                        _stagingBufferManager->Push(nullptr, capacity);
                                        _perFrameCopySizes[_frameIndex].push_back(capacity);
                                    }
                            }
                        const uint32_t stagingOffset = _stagingBufferManager->Push((void*)data->data(), data->size());
                        _perFrameCopySizes[_frameIndex].push_back(data->size());
                        transfer.CopyBuffer(_stagingBuffer.Buffer, destination->Buffer.Buffer, data->size(), stagingOffset);
                    }
                else if (it->ImageCommand.has_value())
                    {
                        // Copy mip maps
                        const CopyImageCommand& v = it->ImageCommand.value();

                        uint32_t mipMapsBytes{};
                        for (const auto& mip : v.MipMapCopy)
                            {
                                mipMapsBytes += mip.Data.size();
                            }

                        // If not space left in staging buffer stop copying
                        const auto capacity = _stagingBufferManager->Capacity();
                        if (capacity < mipMapsBytes)
                            {
                                const auto freeSpace = _stagingBufferManager->MaxSize - _stagingBufferManager->Size();
                                if (freeSpace - capacity < mipMapsBytes)
                                    {
                                        break;
                                    }
                                else
                                    {
                                        _stagingBufferManager->Push(nullptr, capacity);
                                        _perFrameCopySizes[_frameIndex].push_back(capacity);
                                    }
                            }
                        for (const auto& mip : v.MipMapCopy)
                            {
                                DImageVulkan*  destination   = static_cast<DImageVulkan*>(v.Destination);
                                const uint32_t stagingOffset = _stagingBufferManager->Push((void*)mip.Data.data(), mip.Data.size());
                                transfer.CopyMipMap(_stagingBuffer.Buffer, destination->Image.Image, VkExtent2D{ mip.Width, mip.Height }, mip.MipLevel, mip.Offset, stagingOffset);
                            }
                    }
            }

        transfer.FinishCommandBuffer();

        // If we couldn't transfer all data print log
        if (it != _transferCommands.end())
            {
                uint32_t bytesLeft{};
                std::for_each(it, _transferCommands.end(), [&bytesLeft](const CopyDataCommand& c) {
                    bytesLeft += c.VertexCommand.has_value() ? c.VertexCommand.value().Data.size() : 0;
                    bytesLeft += c.UniformCommand.has_value() ? c.UniformCommand.value().Data.size() : 0;
                });
                const std::string out = "Warning: Staging buffer could not copy " + std::to_string(bytesLeft) + " bytes. Condider increase the staging buffer size";
                Log(out);
            }

        // Clear all transfer commands we've processed
        _transferCommands.erase(_transferCommands.begin(), it);
    }
}

void
VulkanContext::_submitCommands()
{
    // Reset command pool
    auto& commandPool = _cmdPool[_frameIndex];
    // Queue submit
    auto recordedCommandBuffers = commandPool.GetRecorded();
    if (recordedCommandBuffers.size() > 0)
        {
            // Reset only when there is work submitted otherwise we'll wait indefinetly
            // vkResetFences(Device.Device, 1, &_fence[_frameIndex]);

            Device.SubmitToMainQueue(recordedCommandBuffers, {}, VK_NULL_HANDLE, VK_NULL_HANDLE);
        }
}

void
VulkanContext::_performDeletionQueue()
{
    // Free resource if N frames have passed
    std::for_each(_deletionQueue.begin(), _deletionQueue.end(), [this](FramesWaitToDeletionList& pair) {
        if (pair.first == 0)
            {
                std::for_each(pair.second.begin(), pair.second.end(), [](const DeleteFn& fn) { fn(); });
            }
    });
    // Remove from queue all deleted fn
    _deletionQueue.erase(std::remove_if(_deletionQueue.begin(), _deletionQueue.end(), [](const FramesWaitToDeletionList& pair) { return pair.first == 0; }), _deletionQueue.end());

    // Decrease the frame number to wait
    std::for_each(_deletionQueue.begin(), _deletionQueue.end(), [this](FramesWaitToDeletionList& pair) { pair.first--; });
}

void
VulkanContext::_recreateSwapchainBlocking(DSwapchainVulkan* swapchain)
{
    // We must be sure that the framebuffer is not used anymore before destroying it
    // This can be a valid approach since a game doesn't resize very often
    vkDeviceWaitIdle(Device.Device);

    // Destroy all framebuffers
    std::for_each(swapchain->Framebuffers->Framebuffers.begin(), swapchain->Framebuffers->Framebuffers.end(), [this](const VkFramebuffer& framebuffer) { Device.DestroyFramebuffer(framebuffer); });
    swapchain->Framebuffers->Framebuffers.clear();

    // Destroy all image views
    std::for_each(swapchain->ImageViews.begin(), swapchain->ImageViews.end(), [this](const VkImageView& imageView) { Device.DestroyImageView(imageView); });
    swapchain->ImageViews.clear();

    // Create from old
    {
        VkSwapchainKHR oldSwapchain = swapchain->Swapchain;

        const VkResult result = Device.CreateSwapchainFromSurface(swapchain->Surface, swapchain->Format, swapchain->PresentMode, swapchain->Capabilities, &swapchain->Swapchain, oldSwapchain);
        if (VKFAILED(result))
            {
                throw std::runtime_error(VkUtils::VkErrorString(result));
            }
        Device.DestroySwapchain(oldSwapchain);
        swapchain->Images.clear();
    }

    // Get image views
    swapchain->Images = Device.GetSwapchainImages(swapchain->Swapchain);

    // Create image views
    std::transform(swapchain->Images.begin(), swapchain->Images.end(), std::back_inserter(swapchain->ImageViews), [this, format = swapchain->Format.format](const VkImage& image) {
        VkImageView imageView{};
        const auto  result = Device.CreateImageView(format, image, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, &imageView);
        if (VKFAILED(result))
            {
                throw std::runtime_error("Failed to create swapchain imageView " + std::string(VkUtils::VkErrorString(result)));
            }
        return imageView;
    });

    // Create a framebuffer

    DRenderPassAttachments attachments;
    DRenderPassAttachment  color(VkUtils::convertVkFormat(swapchain->Format.format),
    ESampleBit::COUNT_1_BIT,
    ERenderPassLoad::Clear,
    ERenderPassStore::Store,
    ERenderPassLayout::Undefined,
    ERenderPassLayout::Present,
    EAttachmentReference::COLOR_ATTACHMENT);
    attachments.Attachments.emplace_back(color);

    VkRenderPass renderPass = _createRenderPass(attachments);

    std::transform(swapchain->ImageViews.begin(), swapchain->ImageViews.end(), std::back_inserter(swapchain->Framebuffers->Framebuffers), [this, swapchain, renderPass](VkImageView view) {
        return Device.CreateFramebuffer({ view }, swapchain->Capabilities.currentExtent.width, swapchain->Capabilities.currentExtent.height, renderPass);
    });
}

VkRenderPass
VulkanContext::_createRenderPass(const DRenderPassAttachments& attachments)
{
    const auto     info = ConvertRenderPassAttachmentsToRIVkRenderPassInfo(attachments);
    VkRenderPass   renderPass{};
    const VkResult result = Device.CreateRenderPass(info, &renderPass);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkUtils::VkErrorString(result));
        }
    _renderPasses.emplace(renderPass);

    return renderPass;
}

std::vector<const char*>
VulkanContext::_getInstanceSupportedExtensions(const std::vector<const char*>& extentions)
{
    const auto               validExtensions      = VkUtils::getInstanceExtensionProperties();
    const auto               validExtentionsNames = VkUtils::convertExtensionPropertiesToNames(validExtensions);
    std::vector<const char*> supportedExtensions  = VkUtils::filterInclusive(extentions, validExtentionsNames);
    {
        const std::string out = "Available extensions requested:" + std::to_string(supportedExtensions.size()) + "/" + std::to_string(extentions.size());
        Log(out);

        // Print not supported validation layers
        if (supportedExtensions.size() != extentions.size())
            {
                Log("Unsupported instance extensions");
                std::vector<const char*> unsupportedExtensions = VkUtils::filterExclusive(validExtentionsNames, extentions);
                for (const char* extensionName : unsupportedExtensions)
                    {
                        Log(extensionName);
                    }
            }
    }
    return supportedExtensions;
}

std::vector<const char*>
VulkanContext::_getInstanceSupportedValidationLayers(const std::vector<const char*>& validationLayers)
{
    const std::vector<VkLayerProperties> availableLayers           = VkUtils::getInstanceLayerProperties();
    const auto                           layerNames                = VkUtils::convertLayerPropertiesToNames(availableLayers);
    std::vector<const char*>             supportedValidationLayers = VkUtils::filterInclusive(validationLayers, layerNames);

    {
        const std::string out = "Available validation layers requested:" + std::to_string(supportedValidationLayers.size()) + "/" + std::to_string(validationLayers.size());
        Log(out);
        // Print not supported validation layers
        if (supportedValidationLayers.size() != validationLayers.size())
            {
                Log("Unsupported instance validation layers:");

                std::vector<const char*> unsupportedValidationLayers = VkUtils::filterExclusive(layerNames, validationLayers);
                for (const char* layerName : unsupportedValidationLayers)
                    {
                        Log(layerName);
                    }
            }
    }

    return supportedValidationLayers;
}

VkPhysicalDevice
VulkanContext::_queryBestPhysicalDevice()
{
    std::vector<VkPhysicalDevice> physicalDevices;
    const VkResult                result = VkUtils::enumeratePhysicalDevices(Instance.Instance, physicalDevices);
    if (VKFAILED(result))
        {
            const std::string out = "Failed to enumerate physical devices:" + std::string(VkUtils::VkErrorString(result));
            Warning(out);
            throw std::runtime_error(VkUtils::VkErrorString(result));
        }
    // Select gpu
    const uint32_t deviceIndex = VkUtils::selectPhysycalDeviceOnHighestMemory(physicalDevices);
    return physicalDevices.at(deviceIndex);
}

std::vector<const char*>
VulkanContext::_getDeviceSupportedExtensions(VkPhysicalDevice physicalDevice, const std::vector<const char*>& extentions)
{
    const auto               validExtensions           = VkUtils::getDeviceExtensionProperties(physicalDevice);
    const auto               extensionNames            = VkUtils::convertExtensionPropertiesToNames(validExtensions);
    std::vector<const char*> deviceSupportedExtensions = VkUtils::filterInclusive(extentions, extensionNames);

    const std::string out = "Available device extensions requested:" + std::to_string(deviceSupportedExtensions.size()) + "/" + std::to_string(extentions.size());
    Log(out);

    // Print not supported validation layers
    if (deviceSupportedExtensions.size() != extentions.size())
        {
            Log("Unsupported device extensions:");
            std::vector<const char*> deviceUnsupportedExtensions = VkUtils::filterExclusive(extensionNames, extentions);
            for (const char* extensionName : deviceUnsupportedExtensions)
                {
                    Log(extensionName);
                }
        }

    return deviceSupportedExtensions;
}

std::vector<const char*>
VulkanContext::_getDeviceSupportedValidationLayers(VkPhysicalDevice physicalDevice, const std::vector<const char*>& validationLayers)
{
    const auto               validExtentions                 = VkUtils::getDeviceExtensionProperties(physicalDevice);
    const auto               validExtentionsNames            = VkUtils::convertExtensionPropertiesToNames(validExtentions);
    std::vector<const char*> deviceSupportedValidationLayers = VkUtils::filterInclusive(validationLayers, validExtentionsNames);

    const std::string out = "Available device validation layers requested:" + std::to_string(deviceSupportedValidationLayers.size()) + "/" + std::to_string(validationLayers.size());
    Log(out);

    // Print not supported validation layers
    if (deviceSupportedValidationLayers.size() != validationLayers.size())
        {
            Log("Unsupported device validation layers:");
            std::vector<const char*> deviceUnsupportedValidationLayers = VkUtils::filterExclusive(validExtentionsNames, validationLayers);
            for (const char* layerName : deviceUnsupportedValidationLayers)
                {
                    Log(layerName);
                }
        }

    return deviceSupportedValidationLayers;
}

RIVkRenderPassInfo
ConvertRenderPassAttachmentsToRIVkRenderPassInfo(const DRenderPassAttachments& attachments)
{
    RIVkRenderPassInfo info;
    info.AttachmentDescription.reserve(attachments.Attachments.size());
    info.ColorAttachmentReference.reserve(attachments.Attachments.size());
    info.DepthStencilAttachmentReference.reserve(attachments.Attachments.size());

    for (const DRenderPassAttachment& att : attachments.Attachments)
        {
            VkAttachmentReference ref{};

            ref.attachment = (uint32_t)info.AttachmentDescription.size();
            ref.layout     = VkUtils::convertAttachmentReferenceLayout(att.AttachmentReferenceLayout);

            switch (ref.layout)
                {
                    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                        info.ColorAttachmentReference.emplace_back(ref);
                        break;
                    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
                    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
                    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
                    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
                    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
                    case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
                    case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
                        info.DepthStencilAttachmentReference.emplace_back(ref);
                        break;
                    default:
                        check(0);
                        break;
                }

            VkAttachmentDescription desc{};
            desc.format  = VkUtils::convertFormat(att.Format);
            desc.samples = VkUtils::convertVkSampleCount(att.Samples);
            desc.loadOp  = VkUtils::convertAttachmentLoadOp(att.LoadOP);
            desc.storeOp = VkUtils::convertAttachmentStoreOp(att.StoreOP);
            // Stencil not used right now
            desc.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

            desc.initialLayout = VkUtils::convertRenderPassLayout(att.InitialLayout, VkUtils::isColorFormat(desc.format));
            desc.finalLayout   = VkUtils::convertRenderPassLayout(att.FinalLayout, VkUtils::isColorFormat(desc.format));

            info.AttachmentDescription.emplace_back(desc);
        }
    {
        VkSubpassDescription subpass{};
        subpass.flags                   = 0;
        subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.inputAttachmentCount    = 0;
        subpass.pInputAttachments       = nullptr;
        subpass.colorAttachmentCount    = (uint32_t)info.ColorAttachmentReference.size();
        subpass.pColorAttachments       = info.ColorAttachmentReference.data();
        subpass.pResolveAttachments     = NULL;
        subpass.pDepthStencilAttachment = (info.DepthStencilAttachmentReference.size()) ? info.DepthStencilAttachmentReference.data() : nullptr;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments    = NULL;

        info.SubpassDescription.emplace_back(std::move(subpass));
    }
    {
        VkSubpassDependency dependency{};
        dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass    = 0;
        dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        info.SubpassDependency.emplace_back(std::move(dependency));
    }

    return info;
}

void
VulkanContext::WaitDeviceIdle()
{
    vkDeviceWaitIdle(Device.Device);
}
}