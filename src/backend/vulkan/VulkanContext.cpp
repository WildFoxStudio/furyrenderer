

#include "VulkanContext.h"

#include "ResourceId.h"
#include "ResourceTransfer.h"
#include "UtilsVK.h"

#include <algorithm>
#include <limits>
#include <math.h>
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
    check(id != NULL); // Uninitialized ID
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
        a = (a ^ 61) ^ (a >> 16);
        return a;
    };
    static size_t counter = 0;
    const auto    value   = hash(counter++);
    check(value != 0); // must not be 0
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
                    while (element.Id == FREE || element.Id == PENDING_DESTROY)
                        {
                            element.Id = GenIdentifier();
                        } // Messy but makes sure that id is never NULL
                    check(element.Id != FREE);
                    check(element.Id != PENDING_DESTROY);
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

    const auto attachmentCount = DFramebufferAttachments::MAX_ATTACHMENTS - std::count(att.RenderTargets.begin(), att.RenderTargets.end(), NULL);
    for (size_t i = 0; i < attachmentCount; i++)
        {
            const auto& renderTargetRef = GetResource<DRenderTargetVulkan, EResourceType::RENDER_TARGET, MAX_RESOURCES>(_renderTargets, att.RenderTargets[i]);

            DRenderPassAttachment o(VkUtils::convertVkFormat(renderTargetRef.Image.Format),
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

    // Initialize per frame pipeline layout map to descriptor pool manager
    _pipelineLayoutToDescriptorPool.resize(NUM_OF_FRAMES_IN_FLIGHT);
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

    for (auto& fbo : _framebuffers)
        {
            if (fbo.Id == FREE)
                continue;

            Device._destroyFramebuffer(fbo.Framebuffer);
            fbo.Id = FREE;
        }

#if _DEBUG
    const auto validSwapchainsCount = std::count_if(_swapchains.begin(), _swapchains.end(), [](const DSwapchainVulkan& swapchain) { return IsValidId(swapchain.Id); });
    check(validSwapchainsCount == 0);
    const auto validVertexBufferCount = std::count_if(_vertexBuffers.begin(), _vertexBuffers.end(), [](const DBufferVulkan& buffer) { return IsValidId(buffer.Id); });
    check(validVertexBufferCount == 0);
    const auto validUniformBufferCount = std::count_if(_uniformBuffers.begin(), _uniformBuffers.end(), [](const DBufferVulkan& buffer) { return IsValidId(buffer.Id); });
    check(validUniformBufferCount == 0);
    const auto validFramebufferCount = std::count_if(_framebuffers.begin(), _framebuffers.end(), [](const DFramebufferVulkan& buffer) { return IsValidId(buffer.Id); });
    check(validFramebufferCount == 0);
    const auto validShaderCount = std::count_if(_shaders.begin(), _shaders.end(), [](const DShaderVulkan& shader) { return IsValidId(shader.Id); });
    check(validShaderCount == 0);
#endif

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

    const auto swapchainImages = Device.GetSwapchainImages(vkSwapchain);
    swapchain.ImagesCount      = swapchainImages.size();
    for (size_t i = 0; i < swapchainImages.size(); i++)
        {
            swapchain.ImagesId[i] = _createImageFromVkImage(swapchainImages.at(i), swapchain.Format.format, swapchain.Capabilities.currentExtent.width, swapchain.Capabilities.currentExtent.height);
        }

    // Create render targets
    for (size_t i = 0; i < swapchainImages.size(); i++)
        {
            const auto index           = AllocResource<DRenderTargetVulkan, MAX_RESOURCES>(_renderTargets);
            auto&      renderTargetRef = _renderTargets.at(index);

            const auto& imageRef        = GetResource<DImageVulkan, EResourceType::IMAGE, MAX_RESOURCES>(_images, swapchain.ImagesId[i]);
            renderTargetRef.Image       = imageRef.Image;
            renderTargetRef.ImageAspect = imageRef.ImageAspect;

            const auto result = Device.CreateImageView(imageRef.Image.Format, imageRef.Image.Image, imageRef.ImageAspect, 0, 1, &renderTargetRef.View);
            if (VKFAILED(result))
                {
                    throw std::runtime_error("Failed to create swapchain imageView " + std::string(VkUtils::VkErrorString(result)));
                }

            swapchain.RenderTargetsId[i] = *ResourceId(EResourceType::RENDER_TARGET, renderTargetRef.Id, index);
        }
}

std::vector<uint32_t>
VulkanContext::GetSwapchainRenderTargets(SwapchainId swapchainId)
{
    ResourceId resource(swapchainId);

    check(resource.First() == EResourceType::SWAPCHAIN);

    DSwapchainVulkan& swapchain = GetResource<DSwapchainVulkan, EResourceType::SWAPCHAIN, MAX_RESOURCES>(_swapchains, swapchainId);

    std::vector<uint32_t> images;
    images.resize(swapchain.ImagesCount);

    for (size_t i = 0; i < swapchain.ImagesCount; i++)
        {
            images[i] = swapchain.RenderTargetsId[i];
        }

    return images;
}

bool
VulkanContext::SwapchainAcquireNextImageIndex(SwapchainId swapchainId, uint64_t timeoutNanoseconds, uint32_t sempahoreid, uint32_t* outImageIndex)
{
    DSwapchainVulkan& swapchain    = GetResource<DSwapchainVulkan, EResourceType::SWAPCHAIN, MAX_RESOURCES>(_swapchains, swapchainId);
    DSemaphoreVulkan& semaphoreRef = GetResource<DSemaphoreVulkan, EResourceType::SEMAPHORE, MAX_RESOURCES>(_semaphores, sempahoreid);

    const VkResult result = Device.AcquireNextImage(swapchain.Swapchain, timeoutNanoseconds, outImageIndex, semaphoreRef.Semaphore, VK_NULL_HANDLE);
    /*Spec: https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/vkQueueSubmit.html*/
    /*
    Return Codes

    On success, this command returns true
            VK_SUCCESS

            VK_TIMEOUT

            VK_NOT_READY

            VK_SUBOPTIMAL_KHR

    On failure, this command returns false
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
                return false;
                break;
            case VK_TIMEOUT:
            default:
                throw std::runtime_error(VkUtils::VkErrorString(result));
                break;
        }
    return true;
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

    // Create renderTargetRef view
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

    Device.DestroySwapchain(swapchain.Swapchain);
    Instance.DestroySurface(swapchain.Surface);

    swapchain.Id = FREE;

    for (uint32_t i = 0; i < swapchain.ImagesCount; i++)
        {
            auto& imageRef = GetResource<DImageVulkan, EResourceType::IMAGE, MAX_RESOURCES>(_images, swapchain.ImagesId[i]);
            Device.DestroyImageView(imageRef.View);
            imageRef.Id = FREE;

            const auto renderTargetId{ swapchain.RenderTargetsId[i] };
            auto&      renderTargetRef = GetResource<DRenderTargetVulkan, EResourceType::RENDER_TARGET, MAX_RESOURCES>(_renderTargets, renderTargetId);

            Device.DestroyImageView(renderTargetRef.View);
            renderTargetRef.Id = FREE;

            // Must destroy all framebuffers that reference this render target @TODO find a better algorithm without iterating over the array multiple times
            const auto referenceCount = std::count_if(_framebuffers.begin(), _framebuffers.end(), [renderTargetId](const DFramebufferVulkan& fbo) {
                for (size_t i = 0; i < DFramebufferAttachments::MAX_ATTACHMENTS; i++)
                    {
                        if (fbo.Attachments.RenderTargets[i] == renderTargetId)
                            {
                                return true;
                            }
                    }
                return false;
            });
            for (uint32_t i = 0; i < referenceCount; i++)
                {
                    auto foundFbo = std::find_if(_framebuffers.begin(), _framebuffers.end(), [renderTargetId](const DFramebufferVulkan& fbo) {
                        for (size_t i = 0; i < DFramebufferAttachments::MAX_ATTACHMENTS; i++)
                            {
                                if (fbo.Attachments.RenderTargets[i] == renderTargetId)
                                    {
                                        return true;
                                    }
                            }
                        return false;
                    });

                    Device._destroyFramebuffer(foundFbo->Framebuffer);
                    foundFbo->Id = FREE;
                }
        }
    swapchain.ImagesCount = 0;

    swapchain.Id = FREE;
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
            case EResourceType::TRANSFER:
                index      = AllocResource<DBufferVulkan, MAX_RESOURCES>(_transferBuffers);
                usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                buffer     = &_transferBuffers.at(index);
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
            case EResourceType::TRANSFER:
                bufferPtr = &_transferBuffers.at(index);
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
            case EResourceType::TRANSFER:
                bufferPtr = &_transferBuffers.at(index);
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
            case EResourceType::TRANSFER:
                bufferPtr = &_transferBuffers.at(index);
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

    const VkImageUsageFlags fomatAttachment = 0; // VkUtils::isColorFormat(vkFormat) ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image.Image                             = Device.CreateImageDeviceLocal(width,
    height,
    mipMapCount,
    vkFormat,
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | fomatAttachment | VK_IMAGE_USAGE_SAMPLED_BIT,
    VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_LAYOUT_UNDEFINED);

    image.ImageAspect = VkUtils::isColorFormat(vkFormat) ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;

    const VkResult result = Device.CreateImageView(vkFormat, image.Image.Image, image.ImageAspect, 0, mipMapCount, &image.View);
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
VulkanContext::CreatePipeline(const ShaderId shader, uint32_t rootSignatureId, const DFramebufferAttachments& attachments, const PipelineFormat& format)
{
    const auto       index = AllocResource<DPipelineVulkan, MAX_RESOURCES>(_pipelines);
    DPipelineVulkan& pso   = _pipelines.at(index);

    const auto&           shaderRef     = GetResource<DShaderVulkan, EResourceType::SHADER, MAX_RESOURCES>(_shaders, shader);
    const DRootSignature& rootSignature = GetResource<DRootSignature, EResourceType::ROOT_SIGNATURE, MAX_RESOURCES>(_rootSignatures, rootSignatureId);

    pso.PipelineLayout = &shaderRef.PipelineLayout;

    auto  rpAttachments = _createGenericRenderPassAttachments(attachments);
    auto  renderPassVk  = _createRenderPass(rpAttachments);
    auto& vertexLayout  = GetResource<DVertexInputLayoutVulkan, EResourceType::VERTEX_INPUT_LAYOUT, MAX_RESOURCES>(_vertexLayouts, shaderRef.VertexLayout);
    pso.Pipeline        = _createPipeline(rootSignature.PipelineLayout, renderPassVk, shaderRef.ShaderStageCreateInfo, format, vertexLayout, shaderRef.VertexStride);

    return *ResourceId(EResourceType::GRAPHICS_PIPELINE, pso.Id, index);
}

void
VulkanContext::DestroyPipeline(uint32_t pipelineId)
{
    auto& pipelineRef = GetResource<DPipelineVulkan, EResourceType::GRAPHICS_PIPELINE, MAX_RESOURCES>(_pipelines, pipelineId);

    Device.DestroyPipeline(pipelineRef.Pipeline);
    pipelineRef.Id = FREE;
}

uint32_t
VulkanContext::CreateRootSignature(const ShaderLayout& layout)
{
    check(layout.SetsLayout.size() < (uint32_t)EDescriptorFrequency::MAX_COUNT);

    const auto      index         = AllocResource<DRootSignature, MAX_RESOURCES>(_rootSignatures);
    DRootSignature& rootSignature = _rootSignatures.at(index);

    std::vector<VkDescriptorSetLayout>        descriptorSetLayout;
    std::map<uint32_t, VkDescriptorSetLayout> setIndexToSetLayout;
    for (const auto& setPair : layout.SetsLayout)
        {
            const auto descriptorSetBindings                  = VkUtils::convertDescriptorBindings(setPair.second);
            rootSignature.DescriptorSetLayouts[setPair.first] = Device.CreateDescriptorSetLayout(descriptorSetBindings);
            descriptorSetLayout.push_back(rootSignature.DescriptorSetLayouts[setPair.first]);

            // Compute pool sizes
            std::map<uint32_t, std::map<uint32_t, ShaderDescriptorBindings>> setBindings;
            setBindings.insert(setPair);
            rootSignature.PoolSizes[setPair.first] = VkUtils::computeDescriptorSetsPoolSize(setBindings);
        }

    // Can return already cached pipeline layout if exists
    rootSignature.PipelineLayout = Device.CreatePipelineLayout(descriptorSetLayout, {});

    rootSignature.SetsBindings = layout.SetsLayout;

    return *ResourceId(EResourceType::ROOT_SIGNATURE, rootSignature.Id, index);
}

void
VulkanContext::DestroyRootSignature(uint32_t rootSignatureId)
{
    DRootSignature& rootSignature = GetResource<DRootSignature, EResourceType::ROOT_SIGNATURE, MAX_RESOURCES>(_rootSignatures, rootSignatureId);

    Device.DestroyPipelineLayout(rootSignature.PipelineLayout);

    for (uint32_t i = 0; i < (uint32_t)EDescriptorFrequency::MAX_COUNT; i++)
        {
            if (rootSignature.DescriptorSetLayouts[i] != nullptr)
                {
                    Device.DestroyDescriptorSetLayout(rootSignature.DescriptorSetLayouts[i]);
                }
        }

    rootSignature.Id = FREE;
    memset(rootSignature.DescriptorSetLayouts, NULL, sizeof(rootSignature.DescriptorSetLayouts));
    for (auto it = 0; it < (uint32_t)EDescriptorFrequency::MAX_COUNT; it++)
        {
            rootSignature.PoolSizes[it].clear();
        }

    rootSignature.SetsBindings.clear();
}

uint32_t
VulkanContext::CreateDescriptorSets(uint32_t rootSignatureId, EDescriptorFrequency frequency, uint32_t count)
{
    DRootSignature& rootSignature = GetResource<DRootSignature, EResourceType::ROOT_SIGNATURE, MAX_RESOURCES>(_rootSignatures, rootSignatureId);

    const auto index            = AllocResource<DDescriptorSet, MAX_RESOURCES>(_descriptorSets);
    auto&      descriptorSetRef = _descriptorSets.at(index);

    descriptorSetRef.Bindings       = rootSignature.SetsBindings[(uint32_t)frequency];
    descriptorSetRef.RootSignature  = &GetResource<DRootSignature, EResourceType::ROOT_SIGNATURE, MAX_RESOURCES>(_rootSignatures, rootSignatureId);
    descriptorSetRef.DescriptorPool = Device.CreateDescriptorPool(rootSignature.PoolSizes[(uint32_t)frequency], count);
    descriptorSetRef.Sets.resize(count);
    descriptorSetRef.Frequency = frequency;

    check(count < 8196);
    std::array<VkDescriptorSetLayout, 8196> descriptorSetLayouts;
    std::fill(descriptorSetLayouts.begin(), descriptorSetLayouts.begin() + count, rootSignature.DescriptorSetLayouts[(uint32_t)frequency]);
    {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = descriptorSetRef.DescriptorPool;
        allocInfo.descriptorSetCount = (uint32_t)count;
        allocInfo.pSetLayouts        = descriptorSetLayouts.data();

        VkDescriptorSet descriptorSet{};
        const VkResult  result = vkAllocateDescriptorSets(Device.Device, &allocInfo, descriptorSetRef.Sets.data());
        if (VKFAILED(result))
            {
                throw std::runtime_error(VkUtils::VkErrorString(result));
            }
    }

    return *ResourceId(EResourceType::DESCRIPTOR_SET, descriptorSetRef.Id, index);
}

void
VulkanContext::DestroyDescriptorSet(uint32_t descriptorSetId)
{
    DDescriptorSet& descriptorSetRef = GetResource<DDescriptorSet, EResourceType::DESCRIPTOR_SET, MAX_RESOURCES>(_descriptorSets, descriptorSetId);
    Device.DestroyDescriptorPool(descriptorSetRef.DescriptorPool);

    descriptorSetRef.Sets.clear();

    descriptorSetRef.Id = FREE;
}

void
VulkanContext::UpdateDescriptorSet(uint32_t descriptorSetId, uint32_t setIndex, uint32_t paramCount, DescriptorData* params)
{
    const DDescriptorSet& descriptorSetRef = GetResource<DDescriptorSet, EResourceType::DESCRIPTOR_SET, MAX_RESOURCES>(_descriptorSets, descriptorSetId);

    check(paramCount < 8196);
    std::array<VkWriteDescriptorSet, 8196>   write;
    std::array<VkDescriptorImageInfo, 8196>  imageInfo;
    std::array<VkDescriptorBufferInfo, 8196> bufferInfo;
    uint32_t                                 writeSetCount{};
    uint32_t                                 imageInfoCount{};
    uint32_t                                 bufferInfoCount{};
    for (uint32_t i = 0; i < paramCount; i++)
        {
            DescriptorData* param           = params + i;
            const uint32_t  descriptorCount = std::max(1u, param->Count);

            VkWriteDescriptorSet* writeSet = &write[writeSetCount++];
            writeSet->sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeSet->pNext                = NULL;
            writeSet->descriptorCount      = descriptorCount;

            const ShaderDescriptorBindings& bindingDesc = descriptorSetRef.Bindings.at(param->Index);
            switch (bindingDesc.StorageType)
                {
                    case EBindingType::STORAGE_BUFFER_OBJECT:
                        writeSet->descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                        check(0); // UNSUPPORTED YET
                        break;
                    case EBindingType::UNIFORM_BUFFER_OBJECT:
                        writeSet->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

                        writeSet->pBufferInfo = &bufferInfo[bufferInfoCount];

                        for (uint32_t i = 0; i < descriptorCount; i++)
                            {
                                VkDescriptorBufferInfo& buf    = bufferInfo[bufferInfoCount++];
                                const DBufferVulkan&    bufRef = GetResource<DBufferVulkan, EResourceType::UNIFORM_BUFFER, MAX_RESOURCES>(_uniformBuffers, param->Buffers[i]);
                                buf.buffer                     = bufRef.Buffer.Buffer;
                                buf.offset                     = 0;
                                buf.range                      = VK_WHOLE_SIZE;
                            }
                        break;
                    case EBindingType::TEXTURE:
                        writeSet->descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                        check(0); // UNSUPPORTED YET
                        // writeSet->pImageInfo = &imageInfo[imageInfoCount];
                        break;
                    case EBindingType::SAMPLER:
                        check(0); // UNSUPPORTED YET
                        writeSet->descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                        break;
                }

            writeSet->dstArrayElement = param->ArrayOffset;
            writeSet->dstBinding      = param->Index;
            writeSet->dstSet          = descriptorSetRef.Sets.at(setIndex);
        }

    vkUpdateDescriptorSets(Device.Device, writeSetCount, write.data(), 0, nullptr);
}

uint32_t
VulkanContext::_createFramebuffer(const DFramebufferAttachments& attachments)
{
    const auto          index          = AllocResource<DFramebufferVulkan, MAX_RESOURCES>(_framebuffers);
    DFramebufferVulkan& framebufferRef = _framebuffers.at(index);

    const auto rpAttachments = _createGenericRenderPassAttachments(attachments);
    const auto vkRenderPass  = _createRenderPass(rpAttachments);

    // Extrapolate all the renderTargetRef views
    const auto attachmentCount = DFramebufferAttachments::MAX_ATTACHMENTS - std::count(attachments.RenderTargets.begin(), attachments.RenderTargets.end(), NULL);
    check(attachmentCount > 0); // Must have at least one attachment

    std::vector<VkImageView> imageViewsAttachments;
    imageViewsAttachments.resize(attachmentCount);

    {
        // First attachment must always exists
        const auto& renderTargetRef = GetResource<DRenderTargetVulkan, EResourceType::RENDER_TARGET, MAX_RESOURCES>(_renderTargets, attachments.RenderTargets[0]);
        framebufferRef.Width        = renderTargetRef.Image.Width;
        framebufferRef.Height       = renderTargetRef.Image.Height;
    }

    for (size_t i = 0; i < attachmentCount; i++)
        {
            check(attachments.RenderTargets[i] != NULL);
            const auto& renderTargetRef = GetResource<DRenderTargetVulkan, EResourceType::RENDER_TARGET, MAX_RESOURCES>(_renderTargets, attachments.RenderTargets[i]);
            imageViewsAttachments[i]    = renderTargetRef.View;

            check(renderTargetRef.Image.Width == framebufferRef.Width); // All images must have the same width
            check(renderTargetRef.Image.Height == framebufferRef.Height); // All images must have the same height
            check(renderTargetRef.Image.MipLevels == 1); // Must have only 1 layer
        }

    framebufferRef.Framebuffer = Device._createFramebuffer(imageViewsAttachments, framebufferRef.Width, framebufferRef.Height, vkRenderPass);
    framebufferRef.Attachments = attachments;

    return *ResourceId(EResourceType::FRAMEBUFFER, framebufferRef.Id, index);
}

void
VulkanContext::_destroyFramebuffer(uint32_t framebufferId)
{
    auto& framebufferRef = GetResource<DFramebufferVulkan, EResourceType::FRAMEBUFFER, MAX_RESOURCES>(_framebuffers, framebufferId);

    Device._destroyFramebuffer(framebufferRef.Framebuffer);

    framebufferRef.Id = FREE;
}

uint32_t
VulkanContext::CreateCommandPool()
{
    const auto index          = AllocResource<DCommandPoolVulkan, MAX_RESOURCES>(_commandPools);
    auto&      commandPoolRef = _commandPools.at(index);

    commandPoolRef.Pool = Device.CreateCommandPool2(Device.GetQueueFamilyIndex());

    return *ResourceId(EResourceType::COMMAND_POOL, commandPoolRef.Id, index);
}

void
VulkanContext::DestroyCommandPool(uint32_t commandPoolId)
{
    auto& commandPoolRef = GetResource<DCommandPoolVulkan, EResourceType::COMMAND_POOL, MAX_RESOURCES>(_commandPools, commandPoolId);

    Device.DestroyCommandPool2(commandPoolRef.Pool);

    commandPoolRef.Id = FREE;
}

void
VulkanContext::ResetCommandPool(uint32_t commandPoolId)
{
    auto& commandPoolRef = GetResource<DCommandPoolVulkan, EResourceType::COMMAND_POOL, MAX_RESOURCES>(_commandPools, commandPoolId);
    Device.ResetCommandPool2(commandPoolRef.Pool);
}

uint32_t
VulkanContext::CreateCommandBuffer(uint32_t commandPoolId)
{
    const auto index            = AllocResource<DCommandBufferVulkan, MAX_RESOURCES>(_commandBuffers);
    auto&      commandBufferRef = _commandBuffers.at(index);

    // Reset internals
    {
        commandBufferRef.ActiveRenderPass = nullptr;
        commandBufferRef.IsRecording      = false;
    }

    auto& commandPoolRef = GetResource<DCommandPoolVulkan, EResourceType::COMMAND_POOL, MAX_RESOURCES>(_commandPools, commandPoolId);

    VkCommandBufferAllocateInfo info{};
    info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.pNext              = NULL;
    info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandPool        = commandPoolRef.Pool;
    info.commandBufferCount = (uint32_t)1;

    const VkResult result = vkAllocateCommandBuffers(Device.Device, &info, &commandBufferRef.Cmd);

    if (VKFAILED(result))
        {
            throw std::runtime_error(VkUtils::VkErrorString(result));
        }

    return *ResourceId(EResourceType::COMMAND_BUFFER, commandBufferRef.Id, index);
}

void
VulkanContext::DestroyCommandBuffer(uint32_t commandBufferId)
{
    auto& commandBufferRef = GetResource<DCommandBufferVulkan, EResourceType::COMMAND_BUFFER, MAX_RESOURCES>(_commandBuffers, commandBufferId);
    check(!commandBufferRef.IsRecording); // Must not be in recording state
    commandBufferRef.Id = FREE;
}

void
VulkanContext::BeginCommandBuffer(uint32_t commandBufferId)
{
    auto& commandBufferRef = GetResource<DCommandBufferVulkan, EResourceType::COMMAND_BUFFER, MAX_RESOURCES>(_commandBuffers, commandBufferId);
    check(!commandBufferRef.IsRecording); // Must not be in recording state
    commandBufferRef.IsRecording = true;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBufferRef.Cmd, &beginInfo);
}

void
VulkanContext::EndCommandBuffer(uint32_t commandBufferId)
{
    auto& commandBufferRef = GetResource<DCommandBufferVulkan, EResourceType::COMMAND_BUFFER, MAX_RESOURCES>(_commandBuffers, commandBufferId);

    check(commandBufferRef.IsRecording); // Must be in recording state
    commandBufferRef.IsRecording = false;

    // If previous active render pass end it
    if (commandBufferRef.ActiveRenderPass)
        {
            vkCmdEndRenderPass(commandBufferRef.Cmd);
            commandBufferRef.ActiveRenderPass = nullptr;
        }

    vkEndCommandBuffer(commandBufferRef.Cmd);
}

void
VulkanContext::BindRenderTargets(uint32_t commandBufferId, const DFramebufferAttachments& colorAttachments, const DLoadOpPass& loadOP)
{

    // Find existing framebuffer
    auto framebufferIt = std::find_if(_framebuffers.begin(), _framebuffers.end(), [colorAttachments](const DFramebufferVulkan& fbo) {
        if (fbo.Id == FREE || fbo.Id == PENDING_DESTROY)
            return false;

        return DFramebufferAttachmentEqualFn{}(fbo.Attachments, colorAttachments);
    });

    DFramebufferVulkan* framebufferPtr{};
    if (framebufferIt != _framebuffers.end())
        {
            framebufferPtr = &(*framebufferIt);
        }
    else
        {
            // Create framebuffer from colorAttachments
            const auto framebufferId = _createFramebuffer(colorAttachments);
            framebufferPtr           = &GetResource<DFramebufferVulkan, EResourceType::FRAMEBUFFER, MAX_RESOURCES>(_framebuffers, framebufferId);
        }

    // Create right render pass
    auto renderPassAttachments = _createGenericRenderPassAttachments(colorAttachments);

    VkClearValue clearValues[DFramebufferAttachments::MAX_ATTACHMENTS];
    uint32_t     clearValueIndex{};

    // Process color colorAttachments
    for (size_t i = 0; i < renderPassAttachments.Attachments.size(); i++)
        {
            renderPassAttachments.Attachments.at(i).LoadOP  = loadOP.LoadColor[i];
            renderPassAttachments.Attachments.at(i).StoreOP = loadOP.StoreActionsColor[i];

            renderPassAttachments.Attachments.at(i).InitialLayout = ERenderPassLayout::AsAttachment;
            renderPassAttachments.Attachments.at(i).FinalLayout   = ERenderPassLayout::AsAttachment;

            if (loadOP.LoadColor[i] == ERenderPassLoad::Clear)
                {
                    renderPassAttachments.Attachments.at(i).InitialLayout = ERenderPassLayout::Undefined;

                    clearValues[clearValueIndex].color.float32[0] = loadOP.ClearColor[i].color.float32[0];
                    clearValues[clearValueIndex].color.float32[1] = loadOP.ClearColor[i].color.float32[1];
                    clearValues[clearValueIndex].color.float32[2] = loadOP.ClearColor[i].color.float32[2];
                    clearValues[clearValueIndex].color.float32[3] = loadOP.ClearColor[i].color.float32[3];
                    clearValueIndex++;
                }
        }

    VkRenderPass renderPass = _createRenderPass(renderPassAttachments);

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = renderPass;
    renderPassInfo.framebuffer       = framebufferPtr->Framebuffer;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = { framebufferPtr->Width, framebufferPtr->Height };
    renderPassInfo.clearValueCount   = clearValueIndex;
    renderPassInfo.pClearValues      = clearValues;

    auto& commandBufferRef = GetResource<DCommandBufferVulkan, EResourceType::COMMAND_BUFFER, MAX_RESOURCES>(_commandBuffers, commandBufferId);
    check(commandBufferRef.IsRecording); // Must be in recording state
    // If previous active render pass end it
    if (commandBufferRef.ActiveRenderPass)
        {
            vkCmdEndRenderPass(commandBufferRef.Cmd);
            commandBufferRef.ActiveRenderPass = nullptr;
        }

    vkCmdBeginRenderPass(commandBufferRef.Cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    commandBufferRef.ActiveRenderPass = renderPass;
}

void
VulkanContext::SetViewport(uint32_t commandBufferId, uint32_t x, uint32_t y, uint32_t width, uint32_t height, float znear, float zfar)
{
    auto& commandBufferRef = GetResource<DCommandBufferVulkan, EResourceType::COMMAND_BUFFER, MAX_RESOURCES>(_commandBuffers, commandBufferId);
    check(commandBufferRef.IsRecording); // Must be in recording state
    check(commandBufferRef.ActiveRenderPass); // Must be in a render pass

    // Invert viewport on Y
    VkViewport viewport{ static_cast<float>(x), static_cast<float>(y) + static_cast<float>(height), static_cast<float>(width), -static_cast<float>(height), znear, zfar };

    vkCmdSetViewport(commandBufferRef.Cmd, 0, 1, &viewport);
}

void
VulkanContext::SetScissor(uint32_t commandBufferId, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    auto& commandBufferRef = GetResource<DCommandBufferVulkan, EResourceType::COMMAND_BUFFER, MAX_RESOURCES>(_commandBuffers, commandBufferId);
    check(commandBufferRef.IsRecording); // Must be in recording state
    check(commandBufferRef.ActiveRenderPass); // Must be in a render pass

    VkRect2D rect{ x, y, width, height };
    vkCmdSetScissor(commandBufferRef.Cmd, 0, 1, &rect);
}

void
VulkanContext::BindPipeline(uint32_t commandBufferId, uint32_t pipeline)
{
    auto& commandBufferRef = GetResource<DCommandBufferVulkan, EResourceType::COMMAND_BUFFER, MAX_RESOURCES>(_commandBuffers, commandBufferId);
    check(commandBufferRef.IsRecording); // Must be in recording state
    check(commandBufferRef.ActiveRenderPass); // Must be in a render pass

    auto& pipelineRef = GetResource<DPipelineVulkan, EResourceType::GRAPHICS_PIPELINE, MAX_RESOURCES>(_pipelines, pipeline);

    vkCmdBindPipeline(commandBufferRef.Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineRef.Pipeline);
}

void
VulkanContext::BindVertexBuffer(uint32_t commandBufferId, uint32_t bufferId)
{
    auto& commandBufferRef = GetResource<DCommandBufferVulkan, EResourceType::COMMAND_BUFFER, MAX_RESOURCES>(_commandBuffers, commandBufferId);
    check(commandBufferRef.IsRecording); // Must be in recording state
    check(commandBufferRef.ActiveRenderPass); // Must be in a render pass

    auto& vertexBufRef = GetResource<DBufferVulkan, EResourceType::VERTEX_INDEX_BUFFER, MAX_RESOURCES>(_vertexBuffers, bufferId);

    VkDeviceSize offset{};
    vkCmdBindVertexBuffers(commandBufferRef.Cmd, 0, 1, &vertexBufRef.Buffer.Buffer, &offset);
}

void
VulkanContext::Draw(uint32_t commandBufferId, uint32_t firstVertex, uint32_t count)
{
    auto& commandBufferRef = GetResource<DCommandBufferVulkan, EResourceType::COMMAND_BUFFER, MAX_RESOURCES>(_commandBuffers, commandBufferId);
    check(commandBufferRef.IsRecording); // Must be in recording state
    check(commandBufferRef.ActiveRenderPass); // Must be in a render pass

    vkCmdDraw(commandBufferRef.Cmd, count, 1, firstVertex, 0);
}

void
VulkanContext::BindDescriptorSet(uint32_t commandBufferId, uint32_t setIndex, uint32_t descriptorSetId)
{
    auto& commandBufferRef = GetResource<DCommandBufferVulkan, EResourceType::COMMAND_BUFFER, MAX_RESOURCES>(_commandBuffers, commandBufferId);
    check(commandBufferRef.IsRecording); // Must be in recording state
    check(commandBufferRef.ActiveRenderPass); // Must be in a render pass

    const DDescriptorSet& descriptorSetRef = GetResource<DDescriptorSet, EResourceType::DESCRIPTOR_SET, MAX_RESOURCES>(_descriptorSets, descriptorSetId);

    vkCmdBindDescriptorSets(
    commandBufferRef.Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, descriptorSetRef.RootSignature->PipelineLayout, (uint32_t)descriptorSetRef.Frequency, 1, &descriptorSetRef.Sets[setIndex], 0, nullptr);
}

void
VulkanContext::CopyImage(uint32_t commandId, uint32_t imageId, uint32_t width, uint32_t height, uint32_t mipMapIndex, uint32_t stagingBufferId, uint32_t stagingBufferOffset)
{
    auto& commandBufferRef = GetResource<DCommandBufferVulkan, EResourceType::COMMAND_BUFFER, MAX_RESOURCES>(_commandBuffers, commandId);
    check(commandBufferRef.IsRecording); // Must be in recording state
    check(!commandBufferRef.ActiveRenderPass); // Must be in a render pass

    auto& imageRef = GetResource<DImageVulkan, EResourceType::IMAGE, MAX_RESOURCES>(_images, imageId);
    auto& buffRef  = GetResource<DBufferVulkan, EResourceType::TRANSFER, MAX_RESOURCES>(_transferBuffers, stagingBufferId);

    VkBufferImageCopy region{};
    {
        region.bufferOffset      = stagingBufferOffset;
        region.bufferRowLength   = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel       = mipMapIndex;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = 1;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { width, height, 1 };
    }

    vkCmdCopyBufferToImage(commandBufferRef.Cmd, buffRef.Buffer.Buffer, imageRef.Image.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)1, &region);
}

void
VulkanContext::QueueSubmit(const std::vector<uint32_t>& waitSemaphore, const std::vector<uint32_t>& finishSemaphore, const std::vector<uint32_t>& cmdIds, uint32_t fenceId)
{
    std::vector<VkCommandBuffer> commandBuffers;
    for (auto cmdId : cmdIds)
        {
            auto& commandBufferRef = GetResource<DCommandBufferVulkan, EResourceType::COMMAND_BUFFER, MAX_RESOURCES>(_commandBuffers, cmdId);
            commandBuffers.push_back(commandBufferRef.Cmd);
        }

    std::vector<VkSemaphore>          waitSemaphores;
    std::vector<VkPipelineStageFlags> waitStages;
    for (auto semaphoreId : waitSemaphore)
        {
            auto& semaphoreRef = GetResource<DSemaphoreVulkan, EResourceType::SEMAPHORE, MAX_RESOURCES>(_semaphores, semaphoreId);
            waitSemaphores.push_back(semaphoreRef.Semaphore);

            waitStages.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        }

    std::vector<VkSemaphore> finishSemaphores;
    for (auto semaphoreId : finishSemaphore)
        {
            auto& semaphoreRef = GetResource<DSemaphoreVulkan, EResourceType::SEMAPHORE, MAX_RESOURCES>(_semaphores, semaphoreId);
            finishSemaphores.push_back(semaphoreRef.Semaphore);
        }

    DFenceVulkan* fenceRef{};
    if (fenceId != NULL)
        {
            fenceRef = &GetResource<DFenceVulkan, EResourceType::FENCE, MAX_RESOURCES>(_fences, fenceId);
        }

    VkSubmitInfo submitInfo{};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = (uint32_t)waitSemaphores.size();
    submitInfo.pWaitSemaphores      = waitSemaphores.data();
    submitInfo.pWaitDstStageMask    = waitStages.data();
    submitInfo.commandBufferCount   = (uint32_t)commandBuffers.size();
    submitInfo.pCommandBuffers      = commandBuffers.data();
    submitInfo.signalSemaphoreCount = (uint32_t)finishSemaphores.size();
    submitInfo.pSignalSemaphores    = finishSemaphores.data();

    const VkResult result = vkQueueSubmit(Device.MainQueue, 1, &submitInfo, fenceRef != nullptr ? fenceRef->Fence : VK_NULL_HANDLE);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkUtils::VkErrorString(result));
        }
}

void
VulkanContext::QueuePresent(uint32_t swapchainId, uint32_t imageIndex, const std::vector<uint32_t>& waitSemaphore)
{
    std::vector<VkSemaphore> waitSemaphores;
    for (auto semaphoreId : waitSemaphore)
        {
            auto& semaphoreRef = GetResource<DSemaphoreVulkan, EResourceType::SEMAPHORE, MAX_RESOURCES>(_semaphores, semaphoreId);
            waitSemaphores.push_back(semaphoreRef.Semaphore);
        }

    auto& swapchainRef = GetResource<DSwapchainVulkan, EResourceType::SWAPCHAIN, MAX_RESOURCES>(_swapchains, swapchainId);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = (uint32_t)waitSemaphores.size();
    presentInfo.pWaitSemaphores    = waitSemaphores.data();
    presentInfo.swapchainCount     = (uint32_t)1;
    presentInfo.pSwapchains        = &swapchainRef.Swapchain;
    presentInfo.pImageIndices      = &imageIndex;
    presentInfo.pResults           = nullptr;

    const VkResult result = vkQueuePresentKHR(Device.MainQueue, &presentInfo);
    if (result == VK_SUCCESS || VK_SUBOPTIMAL_KHR)
        {
            return;
        }

    throw std::runtime_error(VkUtils::VkErrorString(result));

    // On success,
    // this command returns VK_SUCCESS

    // VK_SUBOPTIMAL_KHR

    // On failure,
    // this command returns VK_ERROR_OUT_OF_HOST_MEMORY

    // VK_ERROR_OUT_OF_DEVICE_MEMORY

    // VK_ERROR_DEVICE_LOST

    // VK_ERROR_OUT_OF_DATE_KHR

    // VK_ERROR_SURFACE_LOST_KHR

    // VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT
}

uint32_t
VulkanContext::CreateFence(bool signaled)
{
    const auto index    = AllocResource<DFenceVulkan, MAX_RESOURCES>(_fences);
    auto&      fenceRef = _fences.at(index);
    fenceRef.Fence      = Device.CreateFence(signaled);
    fenceRef.IsSignaled = signaled;

    return *ResourceId(EResourceType::FENCE, fenceRef.Id, index);
}

void
VulkanContext::DestroyFence(uint32_t fenceId)
{
    auto& fenceRef = GetResource<DFenceVulkan, EResourceType::FENCE, MAX_RESOURCES>(_fences, fenceId);

    Device.DestroyFence(fenceRef.Fence);

    fenceRef.Id = FREE;
}

bool
VulkanContext::IsFenceSignaled(uint32_t fenceId)
{
    auto& fenceRef = GetResource<DFenceVulkan, EResourceType::FENCE, MAX_RESOURCES>(_fences, fenceId);
    return fenceRef.IsSignaled;
}

void
VulkanContext::WaitForFence(uint32_t fenceId, uint64_t timeoutNanoseconds)
{

    auto& fenceRef = GetResource<DFenceVulkan, EResourceType::FENCE, MAX_RESOURCES>(_fences, fenceId);

    const VkResult result = vkWaitForFences(Device.Device, 1, &fenceRef.Fence, VK_TRUE /*Wait all*/, timeoutNanoseconds);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkUtils::VkErrorString(result));
        }

    fenceRef.IsSignaled = true;
}

void
VulkanContext::ResetFence(uint32_t fenceId)
{
    auto& fenceRef = GetResource<DFenceVulkan, EResourceType::FENCE, MAX_RESOURCES>(_fences, fenceId);

    vkResetFences(Device.Device, 1, &fenceRef.Fence);

    fenceRef.IsSignaled = false;
}

uint32_t
VulkanContext::CreateGpuSemaphore()
{
    const auto index        = AllocResource<DSemaphoreVulkan, MAX_RESOURCES>(_semaphores);
    auto&      semaphoreRef = _semaphores.at(index);

    semaphoreRef.Semaphore = Device.CreateVkSemaphore();
    return *ResourceId(EResourceType::SEMAPHORE, semaphoreRef.Id, index);
}

void
VulkanContext::DestroyGpuSemaphore(uint32_t semaphoreId)
{
    auto& semaphoreRef = GetResource<DSemaphoreVulkan, EResourceType::SEMAPHORE, MAX_RESOURCES>(_semaphores, semaphoreId);
    Device.DestroyVkSemaphore(semaphoreRef.Semaphore);

    semaphoreRef.Id = FREE;
}

uint32_t
VulkanContext::CreateRenderTarget(EFormat format, ESampleBit samples, bool isDepth, uint32_t width, uint32_t height, uint32_t arrayLength, uint32_t mipMapCount, EResourceState initialState)
{
    check(samples == ESampleBit::COUNT_1_BIT);

    VkImageUsageFlags usageFlags{};

    if (isDepth)
        {
            usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        }
    else
        {
            usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        }

    VkImageLayout initialLayout{};

    const VkFormat vkFormat = VkUtils::convertFormat(format);
    bool           hasStencil{};
    if (VkUtils::isColorFormat(vkFormat))
        {
            hasStencil = VkUtils::formatHasStencil(vkFormat);
        }

    switch (initialState)
        {
            case EResourceState::UNDEFINED:
                initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                break;
            case EResourceState::RENDER_TARGET:
                initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                if (!VkUtils::isColorFormat(vkFormat))
                    {
                        initialLayout = VkUtils::formatHasStencil(vkFormat) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
                    }
                break;
            case EResourceState::UNORDERED_ACCESS:
                initialLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL; // Valid?
                check(0); // Verify is correct
                break;
            case EResourceState::DEPTH_WRITE:
                initialLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
                if (hasStencil)
                    {
                        initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                    }
                break;
            case EResourceState::DEPTH_READ:
                initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                if (hasStencil)
                    {
                        initialLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
                    }
                break;
            case EResourceState::COPY_DEST:
                initialLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                break;
            case EResourceState::COPY_SOURCE:
                initialLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                break;
            case EResourceState::GENERAL_READ:
                initialLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
                break;
            case EResourceState::COMMON:
                initialLayout = VK_IMAGE_LAYOUT_GENERAL;
                break;
            case EResourceState::PRESENT:
                initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                break;

            case EResourceState::INDEX_BUFFER:
            case EResourceState::NON_PIXEL_SHADER_RESOURCE:
            case EResourceState::PIXEL_SHADER_RESOURCE:
            case EResourceState::SHADER_RESOURCE:
            case EResourceState::STREAM_OUT:
            case EResourceState::INDIRECT_ARGUMENT:
            case EResourceState::VERTEX_AND_CONSTANT_BUFFER:
            case EResourceState::RAYTRACING_ACCELERATION_STRUCTURE:
            case EResourceState::SHADING_RATE_SOURCE:
            default:
                check(0);
                break;
        }

    const auto index           = AllocResource<DRenderTargetVulkan, MAX_RESOURCES>(_renderTargets);
    auto&      renderTargetRef = _renderTargets.at(index);
    renderTargetRef.Image      = Device.CreateImageDeviceLocal(width, height, mipMapCount, vkFormat, usageFlags, VK_IMAGE_TILING_OPTIMAL, initialLayout);

    renderTargetRef.ImageAspect = VkUtils::isColorFormat(vkFormat) ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;
    if (VkUtils::formatHasStencil(vkFormat))
        {
            renderTargetRef.ImageAspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }

    {
        const VkResult result = Device.CreateImageView(vkFormat, renderTargetRef.Image.Image, renderTargetRef.ImageAspect, 0, mipMapCount, &renderTargetRef.View);
        if (VKFAILED(result))
            {
                throw std::runtime_error("Failed to create image view");
            }
    }

    return *ResourceId(EResourceType::RENDER_TARGET, renderTargetRef.Id, index);
}

void
VulkanContext::ResourceBarrier(uint32_t commandBufferId,
uint32_t                                buffer_barrier_count,
BufferBarrier*                          p_buffer_barriers,
uint32_t                                texture_barrier_count,
TextureBarrier*                         p_texture_barriers,
uint32_t                                rt_barrier_count,
RenderTargetBarrier*                    p_rt_barriers)
{

    std::vector<VkImageMemoryBarrier> imageBarriers;
    imageBarriers.resize(rt_barrier_count + texture_barrier_count);
    uint32_t imageBarrierCount{};

    VkAccessFlags srcAccessFlags = 0;
    VkAccessFlags dstAccessFlags = 0;

    for (uint32_t i = 0; i < texture_barrier_count; ++i)
        {
            TextureBarrier*       pTrans        = &p_texture_barriers[i];
            const DImageVulkan&   imageRef      = GetResource<DImageVulkan, EResourceType::IMAGE, MAX_RESOURCES>(_images, pTrans->ImageId);
            VkImageMemoryBarrier* pImageBarrier = NULL;

            if (EResourceState::UNORDERED_ACCESS == pTrans->CurrentState && EResourceState::UNORDERED_ACCESS == pTrans->NewState)
                {
                    pImageBarrier        = &imageBarriers[imageBarrierCount++]; //-V522
                    pImageBarrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER; //-V522
                    pImageBarrier->pNext = NULL;

                    pImageBarrier->srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                    pImageBarrier->dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
                    pImageBarrier->oldLayout     = VK_IMAGE_LAYOUT_GENERAL;
                    pImageBarrier->newLayout     = VK_IMAGE_LAYOUT_GENERAL;
                }
            else
                {
                    pImageBarrier        = &imageBarriers[imageBarrierCount++];
                    pImageBarrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    pImageBarrier->pNext = NULL;

                    pImageBarrier->srcAccessMask = VkUtils::resourceStateToAccessFlag(pTrans->CurrentState);
                    pImageBarrier->dstAccessMask = VkUtils::resourceStateToAccessFlag(pTrans->NewState);
                    pImageBarrier->oldLayout     = VkUtils::resourceStateToImageLayout(pTrans->CurrentState);
                    pImageBarrier->newLayout     = VkUtils::resourceStateToImageLayout(pTrans->NewState);
                }

            if (pImageBarrier)
                {
                    pImageBarrier->image                           = imageRef.Image.Image;
                    pImageBarrier->subresourceRange.aspectMask     = (VkImageAspectFlags)imageRef.ImageAspect;
                    pImageBarrier->subresourceRange.baseMipLevel   = pTrans->mSubresourceBarrier ? pTrans->mMipLevel : 0;
                    pImageBarrier->subresourceRange.levelCount     = pTrans->mSubresourceBarrier ? 1 : VK_REMAINING_MIP_LEVELS;
                    pImageBarrier->subresourceRange.baseArrayLayer = pTrans->mSubresourceBarrier ? pTrans->mArrayLayer : 0;
                    pImageBarrier->subresourceRange.layerCount     = pTrans->mSubresourceBarrier ? 1 : VK_REMAINING_ARRAY_LAYERS;

                    //if (pTrans->mAcquire && pTrans->mCurrentState != RESOURCE_STATE_UNDEFINED)
                    //    {
                    //        pImageBarrier->srcQueueFamilyIndex = pCmd->pRenderer->mVulkan.mQueueFamilyIndices[pTrans->mQueueType];
                    //        pImageBarrier->dstQueueFamilyIndex = pCmd->pQueue->mVulkan.mVkQueueFamilyIndex;
                    //    }
                    //else if (pTrans->mRelease && pTrans->mCurrentState != RESOURCE_STATE_UNDEFINED)
                    //    {
                    //        pImageBarrier->srcQueueFamilyIndex = pCmd->pQueue->mVulkan.mVkQueueFamilyIndex;
                    //        pImageBarrier->dstQueueFamilyIndex = pCmd->pRenderer->mVulkan.mQueueFamilyIndices[pTrans->mQueueType];
                    //    }
                    //else
                        {
                            pImageBarrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                            pImageBarrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        }

                    srcAccessFlags |= pImageBarrier->srcAccessMask;
                    dstAccessFlags |= pImageBarrier->dstAccessMask;
                }
        }

    for (uint32_t i = 0; i < rt_barrier_count; ++i)
        {
            RenderTargetBarrier* pTrans = &p_rt_barriers[i];

            auto& renderTargetRef = GetResource<DRenderTargetVulkan, EResourceType::RENDER_TARGET, MAX_RESOURCES>(_renderTargets, pTrans->RenderTarget);

            VkImageMemoryBarrier* pImageBarrier = &imageBarriers[imageBarrierCount++];

            if (EResourceState::UNORDERED_ACCESS == pTrans->mCurrentState && EResourceState::UNORDERED_ACCESS == pTrans->mNewState)
                {
                    pImageBarrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    pImageBarrier->pNext = NULL;

                    pImageBarrier->srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                    pImageBarrier->dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
                    pImageBarrier->oldLayout     = VK_IMAGE_LAYOUT_GENERAL;
                    pImageBarrier->newLayout     = VK_IMAGE_LAYOUT_GENERAL;
                }
            else
                {
                    pImageBarrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    pImageBarrier->pNext = NULL;

                    pImageBarrier->srcAccessMask = VkUtils::resourceStateToAccessFlag(pTrans->mCurrentState);
                    pImageBarrier->dstAccessMask = VkUtils::resourceStateToAccessFlag(pTrans->mNewState);
                    pImageBarrier->oldLayout     = VkUtils::resourceStateToImageLayout(pTrans->mCurrentState);
                    pImageBarrier->newLayout     = VkUtils::resourceStateToImageLayout(pTrans->mNewState);
                }

            if (pImageBarrier)
                {
                    pImageBarrier->image                           = renderTargetRef.Image.Image;
                    pImageBarrier->subresourceRange.aspectMask     = (VkImageAspectFlags)renderTargetRef.ImageAspect;
                    pImageBarrier->subresourceRange.baseMipLevel   = pTrans->mSubresourceBarrier ? pTrans->mMipLevel : 0;
                    pImageBarrier->subresourceRange.levelCount     = pTrans->mSubresourceBarrier ? 1 : VK_REMAINING_MIP_LEVELS;
                    pImageBarrier->subresourceRange.baseArrayLayer = pTrans->mSubresourceBarrier ? pTrans->mArrayLayer : 0;
                    pImageBarrier->subresourceRange.layerCount     = pTrans->mSubresourceBarrier ? 1 : VK_REMAINING_ARRAY_LAYERS;

                    // if (pTrans->mAcquire && pTrans->mCurrentState != EResourceState::UNDEFINED)
                    //     {
                    //         pImageBarrier->srcQueueFamilyIndex = pCmd->pRenderer->mVulkan.mQueueFamilyIndices[pTrans->mQueueType];
                    //         pImageBarrier->dstQueueFamilyIndex = pCmd->pQueue->mVulkan.mVkQueueFamilyIndex;
                    //     }
                    // else if (pTrans->mRelease && pTrans->mCurrentState != EResourceState::UNDEFINED)
                    //     {
                    //         pImageBarrier->srcQueueFamilyIndex = pCmd->pQueue->mVulkan.mVkQueueFamilyIndex;
                    //         pImageBarrier->dstQueueFamilyIndex = pCmd->pRenderer->mVulkan.mQueueFamilyIndices[pTrans->mQueueType];
                    //     }
                    // else
                    {
                        pImageBarrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        pImageBarrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    }

                    srcAccessFlags |= pImageBarrier->srcAccessMask;
                    dstAccessFlags |= pImageBarrier->dstAccessMask;
                }
        }

    VkPipelineStageFlags srcStageMask = VkUtils::determinePipelineStageFlags(srcAccessFlags, EQueueType::GRAPHICS);
    VkPipelineStageFlags dstStageMask = VkUtils::determinePipelineStageFlags(dstAccessFlags, EQueueType::GRAPHICS);

    {
        auto& commandBufferRef = GetResource<DCommandBufferVulkan, EResourceType::COMMAND_BUFFER, MAX_RESOURCES>(_commandBuffers, commandBufferId);
        check(commandBufferRef.IsRecording); // Must be in recording state
        // If previous active render pass end it
        if (commandBufferRef.ActiveRenderPass)
            {
                vkCmdEndRenderPass(commandBufferRef.Cmd);
                commandBufferRef.ActiveRenderPass = nullptr;
            }

        vkCmdPipelineBarrier(commandBufferRef.Cmd, srcStageMask, dstStageMask, 0, 0, NULL, 0, NULL, (uint32_t)imageBarriers.size(), imageBarriers.data());
    }
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