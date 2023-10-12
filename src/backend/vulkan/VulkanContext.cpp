
#include "VulkanContext.h"

#include "ResourceId.h"
#include "ResourceTransfer.h"
#include "UtilsVK.h"

#include <algorithm>
#include <limits>
#include <string>

namespace Fox
{

static constexpr uint8_t PENDING_DESTROY = 0xFF;
static constexpr uint8_t FREE            = 0x00;

static bool
IsValidId(uint8_t id)
{
    return id != FREE && id != PENDING_DESTROY;
};

template<class T, EResourceType type, size_t maxSize>
inline T&
GetResource(std::array<T, maxSize>& container, uint32_t id)
{
    const auto resourceId = ResourceId(id);
    check(resourceId.First() == type); // Invalid resource id
    check(resourceId.Value() < maxSize); // Must be less than array size
    T& element = container.at(resourceId.Value());
    check(IsValidId(element.Id)); // The object must be in valid state
    check(element.Id == resourceId.Second()); // The object must have not been destroyed previously and reallocated
    return element;
};

template<class T, EResourceType type, size_t maxSize>
inline const T&
GetResource(const std::array<T, maxSize>& container, uint32_t id)
{
    const auto resourceId = ResourceId(id);
    check(resourceId.First() == type); // Invalid resource id
    check(resourceId.Value() < maxSize); // Must be less than array size
    const T& element = container.at(resourceId.Value());
    check(IsValidId(element.Id)); // The object must be in valid state
    check(element.Id == resourceId.Second()); // The object must have not been destroyed previously and reallocated
    return element;
};

template<class T, EResourceType type, size_t maxSize>
inline T&
GetResourceUnsafe(std::array<T, maxSize>& container, uint32_t id)
{
    const auto resourceId = ResourceId(id);
    check(resourceId.First() == type); // Invalid resource id
    check(resourceId.Value() < maxSize); // Must be less than array size
    T& element = container.at(resourceId.Value());
    return element;
};

/*generate a random number identitier to be used for resources*/
static uint8_t
GenIdentifier()
{
    // It can hash collide, but is not happening in our case because we're also checking the resource type enum
    // This must be used only to non trivial checking if the resource has been reallocated should have a different id
    auto hash = [](uint32_t a) {
        a = (a ^ 61) ^ (a >> 16);
        a = a + (a << 3);
        a = a ^ (a >> 4);
        a = a * 0x27d4eb2d;
        a = a ^ (a >> 15);
        return a;
    };
    static size_t counter = 0;
    const auto    value   = hash(counter++);
    check(value != 0);
    return (uint8_t)value;
}

template<class T, size_t maxSize>
inline size_t
AllocResource(std::array<T, maxSize>& container)
{
    for (size_t i = 0; i < maxSize; i++)
        {
            T& element = container.at(i);
            if (element.Id == NULL)
                {
                    // memset(&element, NULL, sizeof(T));//Only works with POD style
                    element.Id = GenIdentifier();
                    return i;
                }
        }

    throw std::runtime_error("Failed to allocate!");
    return {};
}

DRenderPassAttachments
VulkanContext::_createGenericRenderPassAttachments(const DFramebufferAttachments& att)
{
    DRenderPassAttachments rp;

    const auto attachmentCount = DFramebufferAttachments::MAX_ATTACHMENTS - std::count(att.ImageIds.begin(), att.ImageIds.end(), NULL);
    for (size_t i = 0; i < attachmentCount; i++)
        {
            const auto& image = GetResource<DImageVulkan, EResourceType::IMAGE, MAX_RESOURCES>(_images, att.ImageIds[i]);

            DRenderPassAttachment o(GetImageFormat(att.ImageIds[i]),
            ESampleBit::COUNT_1_BIT,
            ERenderPassLoad::Clear,
            ERenderPassStore::DontCare,
            ERenderPassLayout::Undefined,
            ERenderPassLayout::ShaderReadOnly,
            EAttachmentReference::COLOR_READ_ONLY);

            if (!VkUtils::isColorFormat(VkUtils::convertFormat(o.Format)))
                {
                    o.AttachmentReferenceLayout = EAttachmentReference::DEPTH_STENCIL_READ_ONLY;
                }

            rp.Attachments.emplace_back(std::move(o));
        }
    return rp;
}

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
        for (auto& e : _cmdPool)
            {
                auto instance = std::make_unique<RICommandPoolManager>(Device.CreateCommandPool());
                e             = std::move(instance);
            }
    }
    // Initialize per frame pipeline layout map to descriptor pool manager
    _pipelineLayoutToDescriptorPool.resize(NUM_OF_FRAMES_IN_FLIGHT);

    // Fences per frame
    {
        constexpr bool fenceSignaled = true; // since the loop begins waiting on a fence the fence must be already signaled otherwise it will timeout
        for (auto& e : _fence)
            {
                e = Device.CreateFence(fenceSignaled);
            }
    }

    // Semaphores per frame
    {
        for (auto& e : _workFinishedSemaphores)
            {
                e = Device.CreateVkSemaphore();
            }
    }

    _deletionQueue.reserve(MAX_RESOURCES);
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

    const auto result = Device.Create(Instance, physicalDevice, validDeviceExtensions, deviceFeatures, validDeviceValidationLayers);
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
#if _DEBUG
    const auto validSwapchainsCount = std::count_if(_swapchains.begin(), _swapchains.end(), [](const DSwapchainVulkan& swapchain) { return IsValidId(swapchain.Id); });
    check(validSwapchainsCount == 0);
    const auto validVertexBufferCount = std::count_if(_vertexBuffers.begin(), _vertexBuffers.end(), [](const DBufferVulkan& buffer) { return IsValidId(buffer.Id); });
    check(validVertexBufferCount == 0);
    const auto validUniformBufferCount = std::count_if(_uniformBuffers.begin(), _uniformBuffers.end(), [](const DBufferVulkan& buffer) { return IsValidId(buffer.Id); });
    check(validUniformBufferCount == 0);
    const auto validFramebufferCount = std::count_if(_framebuffers.begin(), _framebuffers.end(), [](const DFramebufferVulkan_DEPRECATED& buffer) { return IsValidId(buffer.Id); });
    check(validFramebufferCount == 0);
    const auto validShaderCount = std::count_if(_shaders.begin(), _shaders.end(), [](const DShaderVulkan& shader) { return IsValidId(shader.Id); });
    check(validShaderCount == 0);
#endif

    vkDeviceWaitIdle(Device.Device);

    // Wait for all fences to signal
    std::for_each(_fence.begin(), _fence.end(), [this](VkFence fence) {
        const VkResult result = vkWaitForFences(Device.Device, 1, &fence, VK_TRUE /*Wait all*/, MAX_FENCE_TIMEOUT);
        if (VKFAILED(result))
            {
                throw std::runtime_error(VkUtils::VkErrorString(result));
            }
    });
    // Destroy fences
    std::for_each(_fence.begin(), _fence.end(), [this](VkFence fence) { Device.DestroyFence(fence); });

    // Free command pools
    std::for_each(_cmdPool.begin(), _cmdPool.end(), [this](const std::unique_ptr<RICommandPoolManager>& pool) { Device.DestroyCommandPool(pool->GetCommandPool()); });

    std::for_each(_workFinishedSemaphores.begin(), _workFinishedSemaphores.end(), [this](VkSemaphore semaphore) { Device.DestroyVkSemaphore(semaphore); });

    // Free pipeline layouts and their descriptor set layouts used to create them
    // for (const auto& pair : _pipelineLayoutToSetIndexDescriptorSetLayout)
    //    {
    //        Device.DestroyPipelineLayout(pair.first);

    //        for (const auto& p : pair.second)
    //            {
    //                Device.DestroyDescriptorSetLayout(p.second);
    //            }
    //    }
    //_pipelineLayoutToSetIndexDescriptorSetLayout.clear();

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

void
VulkanContext::WaitDeviceIdle()
{
    vkDeviceWaitIdle(Device.Device);
}

SwapchainId
VulkanContext::CreateSwapchain(const WindowData* windowData, EPresentMode& presentMode, EFormat& outFormat)
{
    const auto        index     = AllocResource<DSwapchainVulkan, MAX_RESOURCES>(_swapchains);
    DSwapchainVulkan& swapchain = _swapchains.at(index);

    _createSwapchain(swapchain, windowData, presentMode, outFormat);

    return *ResourceId(EResourceType::SWAPCHAIN, swapchain.Id, index);
}

void
VulkanContext::_createSwapchain(DSwapchainVulkan& swapchain, const WindowData* windowData, EPresentMode& presentMode, EFormat& outFormat)
{
    // Get surface of the window
    VkSurfaceKHR surface{};
    {
        const auto result = Instance.CreateSurfaceFromWindow(*windowData, &surface);
        if (VKFAILED(result))
            {
                throw std::runtime_error("Failed to create surface from window " + std::string(VkUtils::VkErrorString(result)));
            }
    }

    // Query formats present mode and capabilities
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

    // Create swapchain object
    VkSwapchainKHR vkSwapchain{};
    {
        const auto result = Device.CreateSwapchainFromSurface(surface, formats.at(0), vkPresentMode, capabilities, &vkSwapchain);
        if (VKFAILED(result))
            {
                throw std::runtime_error("Failed to create swapchain " + std::string(VkUtils::VkErrorString(result)));
            }
    }

    // Setup new object
    swapchain.Surface      = surface;
    swapchain.Capabilities = capabilities;
    swapchain.Format       = formats.at(0);
    swapchain.PresentMode  = vkPresentMode;
    swapchain.Swapchain    = vkSwapchain;
    swapchain.Images       = Device.GetSwapchainImages(vkSwapchain);

    std::transform(swapchain.Images.begin(), swapchain.Images.end(), std::back_inserter(swapchain.ImageViews), [this, format = swapchain.Format.format](const VkImage& image) {
        VkImageView imageView{};
        const auto  result = Device.CreateImageView(format, image, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, &imageView);
        if (VKFAILED(result))
            {
                throw std::runtime_error("Failed to create swapchain imageView " + std::string(VkUtils::VkErrorString(result)));
            }
        return imageView;
    });

    const auto swapchainImages = Device.GetSwapchainImages(vkSwapchain);
    swapchain.ImagesCount      = swapchainImages.size();
    for (size_t i = 0; i < swapchainImages.size(); i++)
        {
            swapchain.ImagesId[i] = _createImageFromVkImage(swapchainImages.at(i), swapchain.Format.format, swapchain.Capabilities.currentExtent.width, swapchain.Capabilities.currentExtent.height);
        }

    std::transform(swapchain.Images.begin(), swapchain.Images.end(), std::back_inserter(swapchain.ImageViews), [this, format = swapchain.Format.format](const VkImage& image) {
        VkImageView imageView{};
        const auto  result = Device.CreateImageView(format, image, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, &imageView);
        if (VKFAILED(result))
            {
                throw std::runtime_error("Failed to create swapchain imageView " + std::string(VkUtils::VkErrorString(result)));
            }
        return imageView;
    });

    std::generate_n(std::back_inserter(swapchain.ImageAvailableSemaphore), NUM_OF_FRAMES_IN_FLIGHT, [this]() { return Device.CreateVkSemaphore(); });
}

std::vector<ImageId>
VulkanContext::GetSwapchainImages(SwapchainId swapchainId)
{
    ResourceId resource(swapchainId);

    check(resource.First() == EResourceType::SWAPCHAIN);

    DSwapchainVulkan& swapchain = GetResource<DSwapchainVulkan, EResourceType::SWAPCHAIN, MAX_RESOURCES>(_swapchains, swapchainId);

    std::vector<ImageId> images;
    images.resize(swapchain.ImagesCount);

    std::copy(swapchain.ImagesId.begin(), swapchain.ImagesId.begin() + swapchain.ImagesCount, images.data());

    return images;
}

uint32_t
VulkanContext::_createImageFromVkImage(VkImage vkimage, VkFormat format, uint32_t width, uint32_t height)
{

    const auto    index    = AllocResource<DImageVulkan, MAX_RESOURCES>(_images);
    DImageVulkan& image    = _images.at(index);
    image.Image.Image      = vkimage;
    image.Image.Allocation = nullptr;
    image.Image.Format     = format;
    image.Image.Width      = width;
    image.Image.Height     = height;
    image.Image.MipLevels  = 1;
    image.Image.UsageFlags = NULL;

    // Create image view
    image.ImageAspect = VkUtils::isColorFormat(format) ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;
    {
        const VkResult result = Device.CreateImageView(format, image.Image.Image, image.ImageAspect, 0, 1, &image.View);
        if (VKFAILED(result))
            {
                throw std::runtime_error(VkUtils::VkErrorString(result));
            }
    }

    return *ResourceId(EResourceType::IMAGE, image.Id, index);
}

void
VulkanContext::DestroySwapchain(SwapchainId swapchainId)
{
    ResourceId resource(swapchainId);

    check(resource.First() == EResourceType::SWAPCHAIN);

    DSwapchainVulkan& swapchain = GetResource<DSwapchainVulkan, EResourceType::SWAPCHAIN, MAX_RESOURCES>(_swapchains, swapchainId);

    check(swapchain.Framebuffers == NULL); // Must destroy framebuffer first
    {
        vkDeviceWaitIdle(Device.Device);

        swapchain.Images.clear();

        std::for_each(swapchain.ImageViews.begin(), swapchain.ImageViews.end(), [this](const VkImageView& imageView) { Device.DestroyImageView(imageView); });
        swapchain.ImageViews.clear();

        std::for_each(swapchain.ImageAvailableSemaphore.begin(), swapchain.ImageAvailableSemaphore.end(), [this](const VkSemaphore& semaphore) { Device.DestroyVkSemaphore(semaphore); });
        swapchain.ImageAvailableSemaphore.clear();

        Device.DestroySwapchain(swapchain.Swapchain);
        Instance.DestroySurface(swapchain.Surface);

        swapchain.Id = NULL;
    }

    for (uint32_t i = 0; i < swapchain.ImagesCount; i++)
        {
            const auto image = GetResource<DImageVulkan, EResourceType::IMAGE, MAX_RESOURCES>(_images, swapchain.ImagesId[i]);
            Device.DestroyImageView(image.View);
        }
    swapchain.ImagesCount = 0;
}

FramebufferId
VulkanContext::CreateSwapchainFramebuffer_DEPRECATED(SwapchainId swapchainId)
{

    const auto                     index       = AllocResource<DFramebufferVulkan_DEPRECATED, MAX_RESOURCES>(_framebuffers);
    DFramebufferVulkan_DEPRECATED& framebuffer = _framebuffers.at(index);

    DSwapchainVulkan& swapchain = GetResource<DSwapchainVulkan, EResourceType::SWAPCHAIN, MAX_RESOURCES>(_swapchains, swapchainId);
    swapchain.Framebuffers      = *ResourceId(EResourceType::FRAMEBUFFER, framebuffer.Id, index);

    _createFramebufferFromSwapchain(swapchain);

    return swapchain.Framebuffers;
}

void
VulkanContext::_createFramebufferFromSwapchain(DSwapchainVulkan& swapchain)
{
    const auto  swapchainFormat     = swapchain.Format.format;
    const auto& swapchainImageViews = swapchain.ImageViews;
    const auto& capabilities        = swapchain.Capabilities;

    std::vector<VkFormat> attachments{ swapchainFormat };

    DFramebufferVulkan_DEPRECATED& framebuffer = GetResource<DFramebufferVulkan_DEPRECATED, EResourceType::FRAMEBUFFER, MAX_RESOURCES>(_framebuffers, swapchain.Framebuffers);
    framebuffer.Attachments.Attachments.push_back(DRenderPassAttachment(VkUtils::convertVkFormat(swapchainFormat),
    ESampleBit::COUNT_1_BIT,
    ERenderPassLoad::Load,
    ERenderPassStore::DontCare,
    ERenderPassLayout::Undefined,
    ERenderPassLayout::Undefined,
    EAttachmentReference::COLOR_ATTACHMENT));

    framebuffer.RenderPassInfo = _computeFramebufferAttachmentsRenderPassInfo(attachments);

    VkRenderPass renderPass = _createRenderPassFromInfo(framebuffer.RenderPassInfo);

    std::transform(swapchainImageViews.begin(), swapchainImageViews.end(), std::back_inserter(framebuffer.Framebuffers), [this, &capabilities, renderPass](VkImageView view) {
        std::vector<VkImageView> views{ view };
        return Device.CreateFramebuffer(views, capabilities.currentExtent.width, capabilities.currentExtent.height, renderPass);
    });
    framebuffer.Width  = capabilities.currentExtent.width;
    framebuffer.Height = capabilities.currentExtent.height;
}

void
VulkanContext::DestroyFramebuffer(FramebufferId framebufferId)
{
    auto& framebuffer = GetResource<DFramebufferVulkan_DEPRECATED, EResourceType::FRAMEBUFFER, MAX_RESOURCES>(_framebuffers, framebufferId);
    check(IsValidId(framebuffer.Id));
    framebuffer.Id = PENDING_DESTROY;

    // Find if the framebuffer is from a swapchain, remove it from the swapchain
    auto foundSwapchain = std::find_if(_swapchains.begin(), _swapchains.end(), [framebufferId](const DSwapchainVulkan& swapchain) { return swapchain.Framebuffers == framebufferId; });
    if (foundSwapchain != _swapchains.end())
        {
            foundSwapchain->Framebuffers = NULL;
        }

    _deferDestruction([this, framebufferId]() {
        DFramebufferVulkan_DEPRECATED& framebuffer = GetResourceUnsafe<DFramebufferVulkan_DEPRECATED, EResourceType::FRAMEBUFFER, MAX_RESOURCES>(_framebuffers, framebufferId);

        std::for_each(framebuffer.Framebuffers.begin(), framebuffer.Framebuffers.end(), [this](const VkFramebuffer& framebuffer) { Device.DestroyFramebuffer(framebuffer); });
        framebuffer.Framebuffers.clear();

        framebuffer.Id = FREE;
    });
}

BufferId
VulkanContext::CreateBuffer(uint32_t size, EResourceType type, EMemoryUsage usage)
{
    size_t             index{};
    DBufferVulkan*     buffer{};
    VkBufferUsageFlags usageFlags{};
    switch (type)
        {
            case EResourceType::UNIFORM_BUFFER:
                usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
                index      = AllocResource<DBufferVulkan, MAX_RESOURCES>(_uniformBuffers);
                buffer     = &_uniformBuffers.at(index);
                break;
            case EResourceType::VERTEX_INDEX_BUFFER:
                index      = AllocResource<DBufferVulkan, MAX_RESOURCES>(_vertexBuffers);
                usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
                buffer     = &_vertexBuffers.at(index);
                break;
            default:
                check(0); // Invalid type
                break;
        }

    buffer->Size = size;

    switch (usage)
        {
            case EMemoryUsage::RESOURCE_MEMORY_USAGE_GPU_ONLY:
                {
                    buffer->Buffer = Device.CreateBufferDeviceLocalTransferBit(size, usageFlags);
                }
                break;
            case EMemoryUsage::RESOURCE_MEMORY_USAGE_CPU_ONLY:
                {
                    buffer->Buffer = Device.CreateBufferHostVisible(size, usageFlags);
                }
                break;
            case EMemoryUsage::RESOURCE_MEMORY_USAGE_CPU_TO_GPU:
                {
                    buffer->Buffer = Device.CreateBufferHostVisible(size, usageFlags | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
                }
                break;
            default:
                check(0); // invalid usage
                break;
        }

    return *ResourceId(type, buffer->Id, index);
}

void*
VulkanContext::BeginMapBuffer(BufferId buffer)
{
    const auto     resourceType = ResourceId(buffer).First();
    const auto     index        = ResourceId(buffer).Value();
    DBufferVulkan* bufferPtr{};

    switch (resourceType)
        {
            case EResourceType::UNIFORM_BUFFER:
                bufferPtr = &_uniformBuffers.at(index);
                break;
            case EResourceType::VERTEX_INDEX_BUFFER:
                bufferPtr = &_vertexBuffers.at(index);
                break;
            default:
                check(0); // Invalid type
                break;
        }

    check(bufferPtr->Buffer.IsMappable); // Must be mappable flag
    return Device.MapBuffer(bufferPtr->Buffer);
}

void
VulkanContext::EndMapBuffer(BufferId buffer)
{
    const auto     resourceType = ResourceId(buffer).First();
    const auto     index        = ResourceId(buffer).Value();
    DBufferVulkan* bufferPtr{};

    switch (resourceType)
        {
            case EResourceType::UNIFORM_BUFFER:
                bufferPtr = &_uniformBuffers.at(index);
                break;
            case EResourceType::VERTEX_INDEX_BUFFER:
                bufferPtr = &_vertexBuffers.at(index);
                break;
            default:
                check(0); // Invalid type
                break;
        }

    check(bufferPtr->Buffer.IsMappable); // Must be mappable flag
    return Device.UnmapBuffer(bufferPtr->Buffer);
}

void
VulkanContext::DestroyBuffer(BufferId buffer)
{

    const auto     resourceType = ResourceId(buffer).First();
    const auto     index        = ResourceId(buffer).Value();
    DBufferVulkan* bufferPtr{};

    switch (resourceType)
        {
            case EResourceType::UNIFORM_BUFFER:
                bufferPtr = &_uniformBuffers.at(index);
                break;
            case EResourceType::VERTEX_INDEX_BUFFER:
                bufferPtr = &_vertexBuffers.at(index);
                break;
            default:
                check(0); // Invalid type
                break;
        }
    check(IsValidId(bufferPtr->Id));
    bufferPtr->Id = FREE;
    Device.DestroyBuffer(bufferPtr->Buffer);
}

ImageId
VulkanContext::CreateImage(EFormat format, uint32_t width, uint32_t height, uint32_t mipMapCount)
{

    const auto    index = AllocResource<DImageVulkan, MAX_RESOURCES>(_images);
    DImageVulkan& image = _images.at(index);

    const auto vkFormat = VkUtils::convertFormat(format);

    const VkImageUsageFlags fomatAttachment = VkUtils::isColorFormat(vkFormat) ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image.Image = Device.CreateImageDeviceLocal(width, height, mipMapCount, vkFormat, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | fomatAttachment | VK_IMAGE_USAGE_SAMPLED_BIT);

    VkImageAspectFlags imageAspect = VkUtils::isColorFormat(vkFormat) ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;
    const VkResult     result      = Device.CreateImageView(vkFormat, image.Image.Image, imageAspect, 0, mipMapCount, &image.View);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkUtils::VkErrorString(result));
        }

    // Create default sampler
    image.Sampler = Device.CreateSampler(VK_FILTER_NEAREST, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, 0, mipMapCount, VK_SAMPLER_MIPMAP_MODE_NEAREST, true, 16);

    return *ResourceId(EResourceType::IMAGE, image.Id, index);
}

EFormat
VulkanContext::GetImageFormat(ImageId imageId) const
{
    const DImageVulkan& resource = GetResource<DImageVulkan, EResourceType::IMAGE, MAX_RESOURCES>(_images, imageId);

    return VkUtils::convertVkFormat(resource.Image.Format);
}

void
VulkanContext::DestroyImage(ImageId imageId)
{
    auto& resource = GetResourceUnsafe<DImageVulkan, EResourceType::IMAGE, MAX_RESOURCES>(_images, imageId);
    check(IsValidId(resource.Id));
    resource.Id = PENDING_DESTROY;

    _deferDestruction([this, imageId]() {
        auto& resource = GetResourceUnsafe<DImageVulkan, EResourceType::IMAGE, MAX_RESOURCES>(_images, imageId);

        Device.DestroyImageView(resource.View);
        Device.DestroyImage(resource.Image);
        Device.DestroySampler(resource.Sampler);

        resource.Id = FREE;
    });
}

VertexInputLayoutId
VulkanContext::CreateVertexLayout(const std::vector<VertexLayoutInfo>& info)
{
    const auto                index  = AllocResource<DVertexInputLayoutVulkan, MAX_RESOURCES>(_vertexLayouts);
    DVertexInputLayoutVulkan& layout = _vertexLayouts.at(index);
    layout.VertexInputAttributes.reserve(info.size());

    unsigned int location = 0;
    for (const auto& i : info)
        {
            VkVertexInputAttributeDescription attr{};
            attr.location = location++; // uint32_t
            attr.binding  = 0; // uint32_t
            attr.format   = VkUtils::convertFormat(i.Format); // VkFormat
            attr.offset   = i.ByteOffset; // uint32_t

            // Emplace
            layout.VertexInputAttributes.emplace_back(std::move(attr));
        }

    return *ResourceId(EResourceType::VERTEX_INPUT_LAYOUT, layout.Id, index);
}

ShaderId
VulkanContext::CreateShader(const ShaderSource& source)
{
    check(source.ColorAttachments > 0);

    const auto     index  = AllocResource<DShaderVulkan, MAX_RESOURCES>(_shaders);
    DShaderVulkan& shader = _shaders.at(index);
    shader.Id             = GenIdentifier();

    _createShader(source, _shaders.at(index));

    return *ResourceId(EResourceType::SHADER, shader.Id, index);
}

void
VulkanContext::_createShader(const ShaderSource& source, DShaderVulkan& shader)
{
    constexpr uint32_t MAX_SETS_PER_POOL = 100; // Temporary, should not have a limit pool of pools

    shader.VertexLayout           = source.VertexLayout;
    shader.VertexStride           = source.VertexStride;
    shader.ColorAttachments       = source.ColorAttachments;
    shader.DepthStencilAttachment = source.DepthStencilAttachment;

    // Create the shaders
    {

        { const VkResult result = VkUtils::createShaderModule(Device.Device, source.SourceCode.VertexShader, &shader.VertexShaderModule);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkUtils::VkErrorString(result));
        }
    shader.ShaderStageCreateInfo.push_back(VkUtils::createShaderStageInfo(VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, shader.VertexShaderModule));
}

{

    const VkResult result = VkUtils::createShaderModule(Device.Device, source.SourceCode.PixelShader, &shader.PixelShaderModule);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkUtils::VkErrorString(result));
        }
    shader.ShaderStageCreateInfo.push_back(VkUtils::createShaderStageInfo(VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, shader.PixelShaderModule));
}
}

// Create a descriptor set layouts
{
    std::vector<VkDescriptorSetLayout>        descriptorSetLayout;
    std::map<uint32_t, VkDescriptorSetLayout> setIndexToSetLayout;
    for (const auto& setPair : source.SetsLayout.SetsLayout)
        {
            const auto descriptorSetBindings = VkUtils::convertDescriptorBindings(setPair.second);
            descriptorSetLayout.push_back(Device.CreateDescriptorSetLayout(descriptorSetBindings));
            setIndexToSetLayout[setPair.first] = descriptorSetLayout.back();
        }

    shader.DescriptorSetLayouts = std::move(descriptorSetLayout);

    // Can return already cached pipeline layout if exists
    shader.PipelineLayout = Device.CreatePipelineLayout(shader.DescriptorSetLayouts, {});

    // Cache all descriptor sets layouts for this given pipeline layout
    auto pipelineLayoutCacheIt = _pipelineLayoutToSetIndexDescriptorSetLayout.find(shader.PipelineLayout);
    if (pipelineLayoutCacheIt == _pipelineLayoutToSetIndexDescriptorSetLayout.end())
        {
            _pipelineLayoutToSetIndexDescriptorSetLayout[shader.PipelineLayout] = setIndexToSetLayout;

            // Create per frame descriptor pool for this pipeline layout
            const auto poolDimensions = VkUtils::computeDescriptorSetsPoolSize(source.SetsLayout.SetsLayout);
            for (auto i = 0; i < NUM_OF_FRAMES_IN_FLIGHT; i++)
                {
                    auto poolManager = std::make_shared<RIDescriptorPoolManager>(Device.Device, poolDimensions, MAX_SETS_PER_POOL);
                    auto pair        = std::make_pair(shader.PipelineLayout, std::move(poolManager));
                    _pipelineLayoutToDescriptorPool[i].emplace(std::move(pair));
                }
        }
}
}

void
VulkanContext::DestroyShader(const ShaderId shader)
{
    check(ResourceId(shader).First() == EResourceType::SHADER);
    const auto index = ResourceId(shader).Value();
    check(IsValidId(_shaders.at(index).Id));
    _shaders.at(index).Id = PENDING_DESTROY;

    _deferDestruction([this, index]() {
        DShaderVulkan& shaderVulkan = _shaders.at(index);

        vkDestroyShaderModule(Device.Device, shaderVulkan.VertexShaderModule, nullptr);
        vkDestroyShaderModule(Device.Device, shaderVulkan.PixelShaderModule, nullptr);

        // Destroy pipeline, this is unique
        for (const auto& pair : shaderVulkan.RenderPassFormatToPipelinePermutationMap)
            {
                for (const auto& pipeline : pair.second.Pipeline)
                    {
                        Device.DestroyPipeline(pipeline.second);
                    }
            }

        // Find if other shaders share the same pipeline layout
        const auto foundIt =
        std::find_if(_shaders.begin(), _shaders.end(), [shaderVulkan](const DShaderVulkan& shader) { return IsValidId(shader.Id) && (shader.PipelineLayout == shaderVulkan.PipelineLayout); });

        // If not found means that no other shader use the same pipeline layout
        if (foundIt == _shaders.end())
            {
                // We can safely delete this pipeline layout because we're sure is not used anywhere
                Device.DestroyPipelineLayout(shaderVulkan.PipelineLayout);

                // If no pipeline layout reference these descriptor set we can safely delete them
                for (const auto& descSet : shaderVulkan.DescriptorSetLayouts)
                    {
                        // Count how many other shaders have this descriptor set layout
                        const auto useCount = std::count_if(_shaders.begin(), _shaders.end(), [&descSet](const DShaderVulkan& shader) {
                            return std::count_if(shader.DescriptorSetLayouts.begin(), shader.DescriptorSetLayouts.end(), [&descSet](const VkDescriptorSetLayout layout) { return descSet == layout; });
                        });
                        // If this is unique then destroy it
                        if (useCount == 1)
                            {
                                Device.DestroyDescriptorSetLayout(descSet);
                            }
                    }
            }

        _shaders.at(index).Id = FREE;
    });
}

uint32_t
VulkanContext::CreatePipeline(const ShaderId shader, const DFramebufferAttachments& attachments, const PipelineFormat& format)
{
    const auto       index = AllocResource<DPipelineVulkan, MAX_RESOURCES>(_pipelines);
    DPipelineVulkan& pso   = _pipelines.at(index);

    const auto& shaderRef = GetResource<DShaderVulkan, EResourceType::SHADER, MAX_RESOURCES>(_shaders, shader);

    pso.PipelineLayout = &shaderRef.PipelineLayout;

    auto  rpAttachments = _createGenericRenderPassAttachments(attachments);
    auto  renderPassVk  = _createRenderPass(rpAttachments);
    auto& vertexLayout  = GetResource<DVertexInputLayoutVulkan, EResourceType::VERTEX_INPUT_LAYOUT, MAX_RESOURCES>(_vertexLayouts, shaderRef.VertexLayout);
    pso.Pipeline        = _createPipeline(shaderRef.PipelineLayout, renderPassVk, shaderRef.ShaderStageCreateInfo, format, vertexLayout, shaderRef.VertexStride);

    return *ResourceId(EResourceType::GRAPHICS_PIPELINE, pso.Id, index);
}

void
VulkanContext::DestroyPipeline(uint32_t pipelineId)
{
    auto& pipelineRef = GetResource<DPipelineVulkan, EResourceType::GRAPHICS_PIPELINE, MAX_RESOURCES>(_pipelines, pipelineId);

    Device.DestroyPipeline(pipelineRef.Pipeline);
    pipelineRef.Id = FREE;
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
VulkanContext::_createPipeline(VkPipelineLayout     pipelineLayout,
VkRenderPass                                        renderPass,
const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
const PipelineFormat&                               format,
const DVertexInputLayoutVulkan&                     vertexLayout,
uint32_t                                            stride)
{
    // Create pipeline
    VkPipeline graphicsPipeline{};
    {
        // Binding
        const auto vertexInputAttributes = vertexLayout.VertexInputAttributes;

        VkVertexInputBindingDescription inputBinding{};
        inputBinding.binding   = 0;
        inputBinding.stride    = stride;
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
                    pipe.SetPolygonMode(VK_POLYGON_MODE_LINE);
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
    commandPool->Reset();

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
                if (swapchain.Id == NULL)
                    {
                        continue;
                    }

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
                                _recreateSwapchainBlocking(swapchain);
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
                auto cmd = commandPool->Allocate()->Cmd;

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

                        DFramebufferVulkan_DEPRECATED& framebuffer = GetResource<DFramebufferVulkan_DEPRECATED, EResourceType::FRAMEBUFFER, MAX_RESOURCES>(_framebuffers, pass.Framebuffer);
                        {
                            // Bind framebuffer or swapchain framebuffer based on the current image index
                            if (framebuffer.Framebuffers.size() == 1)
                                {
                                    renderPassInfo.framebuffer = framebuffer.Framebuffers.front();
                                }
                            else
                                {
                                    auto swapchain =
                                    std::find_if(_swapchains.begin(), _swapchains.end(), [id = pass.Framebuffer](const DSwapchainVulkan& swapchain) { return swapchain.Framebuffers == id; });
                                    check(swapchain != _swapchains.end());
                                    renderPassInfo.framebuffer = framebuffer.Framebuffers[swapchain->CurrentImageIndex];
                                }
                        }

                        renderPassInfo.renderArea.offset = { 0, 0 };
                        renderPassInfo.renderArea.extent = { framebuffer.Width, framebuffer.Height };

                        check(pass.ClearValues.size() >= std::count_if(pass.RenderPass.Attachments.begin(), pass.RenderPass.Attachments.end(), [](const DRenderPassAttachment& att) {
                            return att.LoadOP == ERenderPassLoad::Clear;
                        })); // Need same number of clear for each clear layout attachament

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
                        // Flip viewport
                        VkViewport viewport{ pass.Viewport.x, pass.Viewport.h - pass.Viewport.y, pass.Viewport.w, -pass.Viewport.h, pass.Viewport.znear, pass.Viewport.zfar };
                        vkCmdSetViewport(cmd, 0, 1, &viewport);
                        VkRect2D scissor{ pass.Viewport.x, pass.Viewport.y, pass.Viewport.w, pass.Viewport.h };
                        vkCmdSetScissor(cmd, 0, 1, &scissor);
                    }
                    // for each draw command
                    {
                        for (const auto& draw : pass.DrawCommands)
                            {
                                auto& shader = GetResource<DShaderVulkan, EResourceType::SHADER, MAX_RESOURCES>(_shaders, draw.Shader);

                                auto pipeline       = _queryPipelineFromAttachmentsAndFormat(shader, pass.RenderPass, draw.PipelineFormat);
                                auto pipelineLayout = shader.PipelineLayout;

                                // Bind pipeline
                                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
                                // find descriptor set from cache and bind them
                                auto descriptorPoolManager = _pipelineLayoutToDescriptorPool[_frameIndex].find(pipelineLayout)->second;

                                // For each set to bind
                                for (const auto& set : draw.DescriptorSetBindings)
                                    {
                                        // Get descriptor set layout for this set
                                        auto descriptorSetLayout = _pipelineLayoutToSetIndexDescriptorSetLayout.find(pipelineLayout)->second[set.first];

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
                                                                DBufferVulkan& buffer = GetResource<DBufferVulkan, EResourceType::UNIFORM_BUFFER, MAX_RESOURCES>(_uniformBuffers, b);
                                                                binder.BindUniformBuffer(bindingIndex, buffer.Buffer.Buffer, 0, buffer.Size);
                                                            }
                                                            break;
                                                        case EBindingType::SAMPLER:
                                                            {
                                                                std::vector<std::pair<VkImageView, VkSampler>> pair;

                                                                std::transform(binding.Images.begin(), binding.Images.end(), std::back_inserter(pair), [this](const ImageId id) {
                                                                    const DImageVulkan& image = GetResource<DImageVulkan, EResourceType::IMAGE, MAX_RESOURCES>(_images, id);
                                                                    return std::make_pair(image.View, image.Sampler);
                                                                });

                                                                binder.BindCombinedImageSamplerArray(bindingIndex, pair);
                                                            }
                                                            break;
                                                        default:
                                                            check(0);
                                                            break;
                                                    }
                                            }

                                        binder.BindDescriptorSet(cmd, pipelineLayout, descriptorSetLayout, set.first);
                                    }

                                // draw call
                                DBufferVulkan& vertexBuffer = GetResource<DBufferVulkan, EResourceType::VERTEX_INDEX_BUFFER, MAX_RESOURCES>(_vertexBuffers, draw.VertexBuffer);
                                VkDeviceSize   offset{ 0 };
                                vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer.Buffer.Buffer, &offset);
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

    _submitCommands(imageAvailableSemaphores);

    // Present
    {
        auto recordedCommandBuffers = commandPool->GetRecorded();

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

                                _recreateSwapchainBlocking(*found);
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
        CResourceTransfer transfer(commandPool->Allocate()->Cmd);
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
                                destination = &GetResource<DBufferVulkan, EResourceType::VERTEX_INDEX_BUFFER, MAX_RESOURCES>(_vertexBuffers, it->VertexCommand.value().Destination);
                                data        = &it->VertexCommand.value().Data;
                                offset      = &it->VertexCommand.value().DestOffset;
                            }
                        if (it->UniformCommand.has_value())
                            {
                                destination = &GetResource<DBufferVulkan, EResourceType::UNIFORM_BUFFER, MAX_RESOURCES>(_uniformBuffers, it->UniformCommand.value().Destination);
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
                                DImageVulkan&  destination   = GetResource<DImageVulkan, EResourceType::IMAGE, MAX_RESOURCES>(_images, v.Destination);
                                const uint32_t stagingOffset = _stagingBufferManager->Push((void*)mip.Data.data(), mip.Data.size());
                                transfer.CopyMipMap(_stagingBuffer.Buffer, destination.Image.Image, VkExtent2D{ mip.Width, mip.Height }, mip.MipLevel, mip.Offset, stagingOffset);
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

VkPipeline
VulkanContext::_queryPipelineFromAttachmentsAndFormat(DShaderVulkan& shader, const DRenderPassAttachments& renderPass, const PipelineFormat& format)
{
    auto& permutationMap = shader.RenderPassFormatToPipelinePermutationMap[renderPass.Attachments];

    auto foundPipeline = permutationMap.Pipeline.find(format);
    if (foundPipeline != permutationMap.Pipeline.end())
        {
            return foundPipeline->second;
        }

    // Create on the fly
    auto       renderPassVk         = _createRenderPass(renderPass);
    auto&      vertexLayout         = GetResource<DVertexInputLayoutVulkan, EResourceType::VERTEX_INPUT_LAYOUT, MAX_RESOURCES>(_vertexLayouts, shader.VertexLayout);
    VkPipeline pipeline             = _createPipeline(shader.PipelineLayout, renderPassVk, shader.ShaderStageCreateInfo, format, vertexLayout, shader.VertexStride);
    permutationMap.Pipeline[format] = pipeline;
    return pipeline;
}

void
VulkanContext::_submitCommands(const std::vector<VkSemaphore>& imageAvailableSemaphores)
{
    // Reset command pool
    auto& commandPool = _cmdPool[_frameIndex];
    // Queue submit
    auto recordedCommandBuffers = commandPool->GetRecorded();
    if (recordedCommandBuffers.size() > 0)
        {
            // Reset only when there is work submitted otherwise we'll wait indefinetly
            vkResetFences(Device.Device, 1, &_fence[_frameIndex]);

            Device.SubmitToMainQueue(recordedCommandBuffers, imageAvailableSemaphores, _workFinishedSemaphores[_frameIndex], _fence[_frameIndex]);
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
VulkanContext::_recreateSwapchainBlocking(DSwapchainVulkan& swapchain)
{
    // We must be sure that the framebuffer is not used anymore before destroying it
    // This can be a valid approach since a game doesn't resize very often
    vkDeviceWaitIdle(Device.Device);

    DFramebufferVulkan_DEPRECATED& framebuffer = GetResource<DFramebufferVulkan_DEPRECATED, EResourceType::FRAMEBUFFER, MAX_RESOURCES>(_framebuffers, swapchain.Framebuffers);
    // Destroy all framebuffers
    {
        std::for_each(framebuffer.Framebuffers.begin(), framebuffer.Framebuffers.end(), [this](const VkFramebuffer& framebuffer) { Device.DestroyFramebuffer(framebuffer); });
        framebuffer.Framebuffers.clear();
    }

    // Destroy all image views
    std::for_each(swapchain.ImageViews.begin(), swapchain.ImageViews.end(), [this](const VkImageView& imageView) { Device.DestroyImageView(imageView); });
    swapchain.ImageViews.clear();

    // Create from old
    {
        // Refresh capabilities
        swapchain.Capabilities = Device.GetSurfaceCapabilities(swapchain.Surface);

        VkSwapchainKHR oldSwapchain = swapchain.Swapchain;

        const VkResult result = Device.CreateSwapchainFromSurface(swapchain.Surface, swapchain.Format, swapchain.PresentMode, swapchain.Capabilities, &swapchain.Swapchain, oldSwapchain);
        if (VKFAILED(result))
            {
                throw std::runtime_error(VkUtils::VkErrorString(result));
            }
        Device.DestroySwapchain(oldSwapchain);
        swapchain.Images.clear();
    }

    // Get image views
    swapchain.Images = Device.GetSwapchainImages(swapchain.Swapchain);

    // Create image views
    std::transform(swapchain.Images.begin(), swapchain.Images.end(), std::back_inserter(swapchain.ImageViews), [this, format = swapchain.Format.format](const VkImage& image) {
        VkImageView imageView{};
        const auto  result = Device.CreateImageView(format, image, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, &imageView);
        if (VKFAILED(result))
            {
                throw std::runtime_error("Failed to create swapchain imageView " + std::string(VkUtils::VkErrorString(result)));
            }
        return imageView;
    });

    // Create a framebuffer
    _createFramebufferFromSwapchain(swapchain);
}

RIVkRenderPassInfo
VulkanContext::_computeFramebufferAttachmentsRenderPassInfo(const std::vector<VkFormat>& attachmentFormat)
{
    DRenderPassAttachments renderPassAttachments;

    for (const auto& format : attachmentFormat)
        {
            DRenderPassAttachment attachment(VkUtils::convertVkFormat(format),
            ESampleBit::COUNT_1_BIT,
            ERenderPassLoad::Clear, // Not important
            ERenderPassStore::Store, // Not important
            ERenderPassLayout::Undefined, // Not important
            ERenderPassLayout::Present, // Not important
            EAttachmentReference::COLOR_ATTACHMENT);

            if (!VkUtils::isColorFormat(format))
                {
                    attachment.AttachmentReferenceLayout = EAttachmentReference::DEPTH_STENCIL_ATTACHMENT;
                }

            renderPassAttachments.Attachments.emplace_back(std::move(attachment));
        }
    return ConvertRenderPassAttachmentsToRIVkRenderPassInfo(renderPassAttachments);
}

VkRenderPass
VulkanContext::_createRenderPass(const DRenderPassAttachments& attachments)
{
    auto info = ConvertRenderPassAttachmentsToRIVkRenderPassInfo(attachments);
    return _createRenderPassFromInfo(info);
}

VkRenderPass
VulkanContext::_createRenderPassFromInfo(const RIVkRenderPassInfo& info)
{
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
}