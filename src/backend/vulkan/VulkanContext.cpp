
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

    // Free command pools
    std::for_each(_cmdPool.begin(), _cmdPool.end(), [this](const RICommandPoolManager& pool) { Device.DestroyCommandPool(pool.GetCommandPool()); });
    _cmdPool.clear();

    _deinitializeStagingBuffer();

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
    swapchainVulkan.Swaphchain   = vkSwapchain;
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

    std::transform(swapchainVulkan.ImageViews.begin(), swapchainVulkan.ImageViews.end(), std::back_inserter(swapchainVulkan.Framebuffers), [this, &capabilities, renderPass](VkImageView view) {
        return Device.CreateFramebuffer({ view }, capabilities.currentExtent.width, capabilities.currentExtent.height, renderPass);
    });

    _swapchains.emplace_back(std::move(swapchainVulkan));

    *swapchain = &_swapchains.back();
    return true;
}

void
VulkanContext::DestroySwapchain(const DSwapchain swapchain)
{
    DSwapchainVulkan* vkSwapchain = static_cast<DSwapchainVulkan*>(swapchain);

    std::for_each(vkSwapchain->Framebuffers.begin(), vkSwapchain->Framebuffers.end(), [this](const VkFramebuffer& framebuffer) { Device.DestroyFramebuffer(framebuffer); });
    vkSwapchain->Framebuffers.clear();
    std::for_each(vkSwapchain->ImageViews.begin(), vkSwapchain->ImageViews.end(), [this](const VkImageView& imageView) { Device.DestroyImageView(imageView); });
    vkSwapchain->ImageViews.clear();

    std::for_each(vkSwapchain->ImageAvailableSemaphore.begin(), vkSwapchain->ImageAvailableSemaphore.end(), [this](const VkSemaphore& semaphore) { Device.DestroyVkSemaphore(semaphore); });
    vkSwapchain->ImageAvailableSemaphore.clear();

    Device.DestroySwapchain(vkSwapchain->Swaphchain);
    Instance.DestroySurface(vkSwapchain->Surface);

    _swapchains.erase(std::find_if(_swapchains.begin(), _swapchains.end(), [swapchain](const DSwapchainVulkan& s) { return &s == swapchain; }));
}

DFramebuffer
VulkanContext::CreateSwapchainFramebuffer()
{
    return nullptr;
}

void
VulkanContext::DestroyFramebuffer(DFramebuffer framebuffer)
{
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

    Device.DestroyBuffer(bufferVulkan->Buffer);

    _vertexBuffers.erase(std::find_if(_vertexBuffers.begin(), _vertexBuffers.end(), [buffer](const DBufferVulkan& b) { return &b == buffer; }));
}

void
VulkanContext::SubmitPass(RenderPassData&& data)
{
}

void
VulkanContext::SubmitCopy(CopyDataCommand&& data)
{
    _transferCommands.emplace_back(std::move(data));
}

void
VulkanContext::AdvanceFrame()
{
    // Reset command pool
    auto& commandPool = _cmdPool[_frameIndex];
    commandPool.Reset();

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
                if (it->VertexCommand.has_value())
                    { // Perform vertex copy
                        const CopyVertexCommand& v = it->VertexCommand.value();

                        // If not space left in staging buffer stop copying
                        const auto capacity = _stagingBufferManager->Capacity();
                        if (capacity < v.Data.size())
                            {
                                const auto freeSpace = _stagingBufferManager->MaxSize - _stagingBufferManager->Size();
                                if (freeSpace - capacity < v.Data.size())
                                    {
                                        break;
                                    }
                                else
                                    {
                                        _stagingBufferManager->Push(nullptr, capacity);
                                        _perFrameCopySizes[_frameIndex].push_back(capacity);
                                    }
                            }
                        DBufferVulkan* destination = static_cast<DBufferVulkan*>(v.Destination);

                        const uint32_t stagingOffset = _stagingBufferManager->Push((void*)v.Data.data(), v.Data.size());
                        _perFrameCopySizes[_frameIndex].push_back(v.Data.size());
                        transfer.CopyVertexBuffer(_stagingBuffer.Buffer, destination->Buffer.Buffer, v.Data.size(), stagingOffset);
                    }
                // else if (it->UniformCommand.has_value())
                //     { // Copy ubo
                //         const CopyUniformBufferCommand& v = it->UniformCommand.value();

                //        // If not space left in staging buffer stop copying
                //        if (!_stagingBufferManager->DoesFit(v.Data.size()))
                //            {
                //                break;
                //            }
                //        const uint32_t stagingOffset = _stagingBufferManager->Copy((void*)v.Data.data(), v.Data.size());
                //        transfer.CopyVertexBuffer(_stagingBuffer.Buffer, v.Destination->Buffer.Buffer, v.Data.size(), stagingOffset);
                //    }
                // else if (it->ImageCommand.has_value())
                //     {
                //         // Copy mip maps
                //         const CopyImageCommand& v = it->ImageCommand.value();

                //        uint32_t mipMapsBytes{};
                //        for (const auto& mip : v.MipMapCopy)
                //            {
                //                mipMapsBytes += mip.Data.size();
                //            }

                //        // If not space left in staging buffer stop copying
                //        if (!_stagingBufferManager->DoesFit(mipMapsBytes))
                //            {
                //                break;
                //            }

                //        for (const auto& mip : v.MipMapCopy)
                //            {
                //                const uint32_t stagingOffset = _stagingBufferManager->Copy((void*)mip.Data.data(), mip.Data.size());
                //                transfer.CopyMipMap(_stagingBuffer.Buffer, v.Destination, VkExtent2D{ mip.Width, mip.Height }, mip.MipLevel, mip.Offset, stagingOffset);
                //            }
                //    }
            }
        // This is used to move tail as same position as head (free space) of previous frame data
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

    // Queue submit
    auto recordedCommandBuffers = commandPool.GetRecorded();
    if (recordedCommandBuffers.size() > 0)
        {
            // Reset only when there is work submitted otherwise we'll wait indefinetly
            // vkResetFences(Device.Device, 1, &_fence[_frameIndex]);

            Device.SubmitToMainQueue(recordedCommandBuffers, {}, VK_NULL_HANDLE, VK_NULL_HANDLE);
        }

    // Increment the frame index
    _frameIndex = (_frameIndex + 1) % NUM_OF_FRAMES_IN_FLIGHT;
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