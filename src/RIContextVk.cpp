// Copyright RedFox Studio 2022

#include "RIContextVk.h"
#include "ResourceTransfer.h"
#include <map>

namespace Fox
{

RIContextVk::RIContextVk()
{
    std::cout << "Initializing Vulkan Context\n";

    {
        const VkResult result = volkInitialize();
        if (result != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to initialize volk");
            }
    }

    // Initialize instance
    auto validValidationLayers = _getInstanceSupportedValidationLayers(_validationLayers);
    auto validExtensions       = _getInstanceSupportedExtensions(_instanceExtensionNames);

    if (!VulkanInstance.Init("Application", validValidationLayers, validExtensions))
        throw std::runtime_error("Could not create a vulkan instance");

    // Print version
    _printVersion();

    // Setup debugger
#ifdef _DEBUG
    VulkanInstance.CreateDebugUtilsMessenger(DebugCallback);
#endif

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

    if (!VulkanDevice.Create(VulkanInstance, physicalDevice, _deviceExtensionNames, deviceFeatures, _validationLayers))
        throw std::runtime_error("Could not create a vulkan device");

    // replacing global function pointers with functions retrieved with vkGetDeviceProcAddr
    volkLoadDevice(VulkanDevice.Device);

    const uint32_t                    MAX = 250000;
    std::vector<VkDescriptorPoolSize> globalPoolSize{
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, MAX }, { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX }, { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX }
    };
    auto pool = VulkanDevice.CreateDescriptorPool(globalPoolSize, MAX);
    _genericDescriptorPools.push_back(pool);
    _globalDescriptorPool = CreateDescriptorPoolManager(pool, MAX);

    // New front end stuff ---------------------------------------------------------------------------------------------------------
    {
        // Command pools per frame
        {
            std::generate_n(std::back_inserter(_cmdPool), GetNumberOfFramesInFlight(), [this]() { return RICommandPoolManager(MT_CreateCommandPool2()); });
        }

        // Fences per frame
        {
            constexpr bool fenceSignaled = true; // since the loop begins waiting on a fence the fence must be already signaled otherwise it will timeout
            std::generate_n(std::back_inserter(_fence), GetNumberOfFramesInFlight(), [this, fenceSignaled = fenceSignaled]() { return VulkanDevice.CreateFence(fenceSignaled); });
        }

        // Semaphores per frame
        {
            std::generate_n(std::back_inserter(_workFinishedSemaphores), GetNumberOfFramesInFlight(), [this]() { return VulkanDevice.CreateVkSemaphore(); });
        }

        // Descriptor pools per frame
        {
            _pipelineLayoutToDescriptorPool.resize(GetNumberOfFramesInFlight());
        }

        _stagingBuffer        = VulkanDevice.CreateBufferHostVisible(STAGING_BUFFER_SIZE, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        _stagingBufferPtr     = (unsigned char*)VulkanDevice.MapBuffer(_stagingBuffer);
        _stagingBufferManager = std::make_unique<RingBufferManager>(STAGING_BUFFER_SIZE, _stagingBufferPtr);

        _uboSsboBufferPool = VulkanDevice.CreateBufferDeviceLocalTransferBit(UBO_SSBO_POOL_BYTES, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        _vertexIndexPool   = VulkanDevice.CreateBufferDeviceLocalTransferBit(VERTEX_INDEX_POOL_BYTES, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    }
}

RIContextVk::~RIContextVk()
{
    std::cerr << "Tearing Vulkan Context\n" << std::flush;

    vkQueueWaitIdle(VulkanDevice.MainQueue);
    vkDeviceWaitIdle(VulkanDevice.Device);

    for (auto func : _cleanupResources)
        {
            func();
        }

    // New front end stuff ---------------------------------------------------------------------------------------------------------
    {
        // Wait for all fences to signal
        std::for_each(_fence.begin(), _fence.end(), [this](VkFence fence) { _waitFence(fence); });

        // Destroy fences
        std::for_each(_fence.begin(), _fence.end(), [this](VkFence fence) { VulkanDevice.DestroyFence(fence); });
        _fence.clear();

        // Free command pools
        std::for_each(_cmdPool.begin(), _cmdPool.end(), [this](const RICommandPoolManager& pool) { MT_DestroyCommandPool2(pool.GetCommandPool()); });
        _cmdPool.clear();

        // Check if all swapchains were destroyed
        check(_windowToSwapchain.size() == 0);

        // Unmap and destroy staging buffer
        _stagingBufferManager.reset();
        _stagingBufferPtr = {};
        VulkanDevice.UnmapBuffer(_stagingBuffer);
        VulkanDevice.DestroyBuffer(_stagingBuffer);
    }
    // New front end stuff ---------------------------------------------------------------------------------------------------------

    VulkanDevice.Deinit();
    VulkanInstance.Deinit();
}

ERI_ERROR
RIContextVk::TryAcquireNextImage(RISwapchain* swapchain, uint32_t* frameIndex, bool& wasRecreated)
{

    wasRecreated               = false;
    RISwapchainVk* riswapchain = static_cast<RISwapchainVk*>(swapchain);
    check(riswapchain->IsValid());

    uint32_t currentFrame = *frameIndex;

    RISemaphoreVk* semaphore = (RISemaphoreVk*)(riswapchain->ImageAvailableSemaphores[currentFrame]);

    VkResult result = VulkanDevice.AcquireNextImage(riswapchain->Swapchain, UINT64_MAX, frameIndex, semaphore->Semaphore);

    /*Spec: https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/vkQueueSubmit.html*/
    /*
    Return Codes

    On success, this command returns

            VK_SUCCESS

    On failure, this command returns

            VK_ERROR_OUT_OF_HOST_MEMORY

            VK_ERROR_OUT_OF_DEVICE_MEMORY

            VK_ERROR_DEVICE_LOST
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
        }

    return ERI_ERROR::SUCCESS;
}

#pragma region FrontEnd rendering commands

bool
RIContextVk::CreateSwapchain(AWindow* window, VkSurfaceFormatKHR format, VkPresentModeKHR presentMode)
{
    {
        auto found = _windowToSwapchain.find(window->GetWindowId());
        check(found == _windowToSwapchain.end());
    }

    SwapchainVk sc;

    sc.Surface.Surface                      = MT_CreateSurfaceFromWindow(window);
    sc.Surface.SurfaceFormatsSupported      = MT_GetSurfaceFormats(sc.Surface.Surface);
    sc.Surface.SurfacePresentModesSupported = MT_GetSurfacePresentModes(sc.Surface.Surface);
    sc.Surface.SurfaceCapabilities          = MT_GetSurfaceCapabilities(sc.Surface.Surface);

    // If format not supported
    {
        const auto it = std::find_if(sc.Surface.SurfaceFormatsSupported.begin(), sc.Surface.SurfaceFormatsSupported.end(), [format](const VkSurfaceFormatKHR& f) {
            return f.format == format.format && f.colorSpace == format.colorSpace;
        });
        if (it == sc.Surface.SurfaceFormatsSupported.end())
            {
                MT_DestroySurface(sc.Surface.Surface);
                return false;
            }
    }
    // If present mode not supported
    {
        const auto it = std::find(sc.Surface.SurfacePresentModesSupported.begin(), sc.Surface.SurfacePresentModesSupported.end(), presentMode);
        if (it == sc.Surface.SurfacePresentModesSupported.end())
            {
                MT_DestroySurface(sc.Surface.Surface);
                return false;
            }
    }

    sc.SwapchainFormat      = format;
    sc.SwapchainPresentMode = presentMode;

    sc.Swapchain = VulkanDevice.CreateSwapchainFromSurface(sc.Surface.Surface, sc.SwapchainFormat, sc.SwapchainPresentMode, sc.Surface.SurfaceCapabilities);

    MT_GetSwapchainImages(sc.Swapchain, sc.SwapchainImages);

    std::transform(sc.SwapchainImages.begin(), sc.SwapchainImages.end(), std::back_inserter(sc.ImageViews), [this, &sc](VkImage img) { return MT_CreateImageView(img, sc.SwapchainFormat.format); });

    std::generate_n(std::back_inserter(sc.ImageAvailableSemaphore), GetNumberOfFramesInFlight(), [this]() { return VulkanDevice.CreateVkSemaphore(); });

    DRenderPassAttachments att;
    att.Attachments.push_back(DRenderPassAttachment(
    sc.SwapchainFormat.format, ERenderPassLoad::Clear, ERenderPassStore::Store, ERenderPassLayout::Undefined, ERenderPassLayout::Present, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));

    VkRenderPass renderPass = MT_CreateRenderPass(att);

    std::transform(sc.ImageViews.begin(), sc.ImageViews.end(), std::back_inserter(sc.Framebuffers), [this, sc, renderPass](VkImageView view) {
        return MT_CreateFramebuffer({ view }, sc.Surface.SurfaceCapabilities.currentExtent, renderPass);
    });

    _windowToSwapchain[window->GetWindowId()] = std::move(sc);

    return true;
}

void
RIContextVk::DestroySwapchain(WindowIdentifier id)
{
    auto found = _windowToSwapchain.find(id);
    check(found != _windowToSwapchain.end());

    WaitGpuIdle();

    // Destroy semaphores
    std::for_each(found->second.ImageAvailableSemaphore.begin(), found->second.ImageAvailableSemaphore.end(), [this](VkSemaphore semaphore) { VulkanDevice.DestroyVkSemaphore(semaphore); });
    found->second.ImageAvailableSemaphore.clear();

    std::for_each(found->second.Framebuffers.begin(), found->second.Framebuffers.end(), [this](VkFramebuffer fbo) { MT_DestroyFramebuffer(fbo); });
    found->second.Framebuffers.clear();

    std::for_each(found->second.ImageViews.begin(), found->second.ImageViews.end(), [this](VkImageView view) { MT_DestroyImageView(view); });
    found->second.ImageViews.clear();

    MT_DestroySwapchain(found->second.Swapchain);

    _windowToSwapchain.erase(found);
}

VkExtent2D
RIContextVk::GetSwapchainExtent(WindowIdentifier id)
{
    auto found = _windowToSwapchain.find(id);
    check(found != _windowToSwapchain.end());
    return found->second.Surface.SurfaceCapabilities.currentExtent;
}

FramebufferRef
RIContextVk::GetSwapchainFramebuffer(WindowIdentifier id)
{
    auto found = _windowToSwapchain.find(id);
    check(found != _windowToSwapchain.end());
    return FramebufferRef{ &found->second };
}

void
RIContextVk::SubmitPass(RenderPassData&& data)
{
    _renderPasses.emplace_back(std::move(data));
}

void
RIContextVk::SubmitCopy(CopyDataCommand&& data)
{
    _transferCommands.emplace_back(std::move(data));
}

void
RIContextVk::AdvanceFrame()
{
    // Wait fence
    _waitFence(_fence[_frameIndex]);

    // Reset command pool
    auto& commandPool = _cmdPool[_frameIndex];
    commandPool.Reset();

    // Perform copy commands
    {
        CResourceTransfer transfer(commandPool.Allocate()->Cmd);
        auto              it = _transferCommands.begin();
        for (; it != _transferCommands.end(); it++)
            {
                if (it->VertexCommand.has_value())
                    { // Perform vertex copy
                        const CopyVertexCommand& v = it->VertexCommand.value();

                        // If not space left in staging buffer stop copying
                        if (!_stagingBufferManager->DoesFit(v.Data.size()))
                            {
                                break;
                            }

                        const uint32_t stagingOffset = _stagingBufferManager->Copy((void*)v.Data.data(), v.Data.size());
                        transfer.CopyVertexBuffer(_stagingBuffer.Buffer, v.Destination, v.Data.size(), stagingOffset);
                    }
                else if (it->UniformCommand.has_value())
                    { // Copy ubo
                        const CopyUniformBufferCommand& v = it->UniformCommand.value();

                        // If not space left in staging buffer stop copying
                        if (!_stagingBufferManager->DoesFit(v.Data.size()))
                            {
                                break;
                            }
                        const uint32_t stagingOffset = _stagingBufferManager->Copy((void*)v.Data.data(), v.Data.size());
                        transfer.CopyVertexBuffer(_stagingBuffer.Buffer, v.Destination->Buffer.Buffer, v.Data.size(), stagingOffset);
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
                        if (!_stagingBufferManager->DoesFit(mipMapsBytes))
                            {
                                break;
                            }

                        for (const auto& mip : v.MipMapCopy)
                            {
                                const uint32_t stagingOffset = _stagingBufferManager->Copy((void*)mip.Data.data(), mip.Data.size());
                                transfer.CopyMipMap(_stagingBuffer.Buffer, v.Destination, VkExtent2D{ mip.Width, mip.Height }, mip.MipLevel, mip.Offset, stagingOffset);
                            }
                    }
            }
        // This is used to move tail as same position as head (free space) of previous frame data
        _stagingBufferManager->SetTail(_stagingBufferManagerSetTailOnNextFrame);
        _stagingBufferManagerSetTailOnNextFrame = _stagingBufferManager->GetHead();
        transfer.FinishCommandBuffer();

        // Clear all transfer commands we've processed
        _transferCommands.erase(_transferCommands.begin(), it);
    }

    // Reset all descriptor pools for this frame
    auto& currentPipelineLayoutToDescriptorPool = _pipelineLayoutToDescriptorPool[_frameIndex];
    for (auto& pair : currentPipelineLayoutToDescriptorPool)
        {
            pair.second->ResetPool();
        }

    // Acquire next image for all swapchains
    // Recreate swapchain if necessary
    std::vector<VkSemaphore> imageAvailableSemaphores;
    imageAvailableSemaphores.reserve(_windowToSwapchain.size());
    std::vector<std::pair<VkSwapchainKHR, uint32_t>> swapchainImageIndex;
    swapchainImageIndex.reserve(_windowToSwapchain.size());

    for (std::pair<const WindowIdentifier, SwapchainVk>& pair : _windowToSwapchain)
        {
            const VkResult result = VulkanDevice.AcquireNextImage(pair.second.Swapchain, UINT64_MAX, &pair.second.CurrentImageIndex, pair.second.ImageAvailableSemaphore[_frameIndex], VK_NULL_HANDLE);
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
                            WaitGpuIdle();
                            _recreateSwapchain(pair.second);
                            const VkResult lastResult =
                            VulkanDevice.AcquireNextImage(pair.second.Swapchain, UINT64_MAX, &pair.second.CurrentImageIndex, pair.second.ImageAvailableSemaphore[_frameIndex], VK_NULL_HANDLE);
                            if (VKFAILED(lastResult))
                                {
                                    throw std::runtime_error(VkErrorString(result));
                                }
                        }
                        break;
                    case VK_TIMEOUT:
                    default:
                        throw std::runtime_error(VkErrorString(result));
                        break;
                }

            imageAvailableSemaphores.push_back(pair.second.ImageAvailableSemaphore[_frameIndex]);
            swapchainImageIndex.push_back(std::make_pair(pair.second.Swapchain, pair.second.CurrentImageIndex));
        }

    // Extract all the descriptor set updates from the draw commands
    {
        // Convert descriptor set updates to vulkan VkWriteDescriptorSet
        // Cache all the descriptor sets
        // Update all descriptor sets
    }

    // For all the passes
    {
        for (const auto& pass : _renderPasses)
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
                        renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                        renderPassInfo.renderPass        = pass.RenderPass;
                        renderPassInfo.framebuffer       = pass.Framebuffer.Swapchain->Framebuffers[pass.Framebuffer.Swapchain->CurrentImageIndex];
                        renderPassInfo.renderArea.offset = { 0, 0 };
                        renderPassInfo.renderArea.extent = { (uint32_t)pass.Viewport.width, (uint32_t)pass.Viewport.height };
                        renderPassInfo.clearValueCount   = (uint32_t)pass.ClearValues.size();
                        renderPassInfo.pClearValues      = pass.ClearValues.data();
                        vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                    }
                    {
                        vkCmdSetViewport(cmd, 0, 1, &pass.Viewport);
                        VkRect2D scissor{ 0, 0, pass.Viewport.width, pass.Viewport.height };
                        vkCmdSetScissor(cmd, 0, 1, &scissor);
                    }
                    // for each draw command
                    {
                        for (const auto& draw : pass.DrawCommands)
                            {
                                // Bind pipeline
                                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.Pipeline.Pipeline);
                                // find descriptor set from cache and bind them
                                auto descriptorPoolManager = _pipelineLayoutToDescriptorPool[_frameIndex].find(draw.Pipeline.PipelineLayout)->second;

                                // For each set to bind
                                for (const auto& set : draw.DescriptorSetBindings)
                                    {
                                        // Get descriptor set layout for this set
                                        auto descriptorSetLayout = _pipelineLayoutToSetIndexDescriptorSetLayout.find(draw.Pipeline.PipelineLayout)->second[set.first];

                                        auto binder = descriptorPoolManager->CreateDescriptorSetBinder();
                                        for (const auto& bindingPair : set.second)
                                            {
                                                const auto  bindingIndex = bindingPair.first;
                                                const auto& binding      = bindingPair.second;
                                                switch (binding.Type)
                                                    {
                                                        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                                                            {
                                                                check(binding.Buffers.size() == 1);
                                                                const auto& b = binding.Buffers.front();
                                                                binder.BindUniformBuffer(bindingIndex, b.Buffer, b.Offset, b.Range);
                                                            }
                                                            break;
                                                        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                                                            {
                                                                std::vector<std::pair<VkImageView, VkSampler>> pair;

                                                                std::transform(binding.Images.begin(), binding.Images.end(), std::back_inserter(pair), [](const SetImage& i) {
                                                                    return std::make_pair(i.ImageView, i.Sampler);
                                                                });

                                                                binder.BindCombinedImageSamplerArray(bindingIndex, pair);
                                                            }
                                                            break;
                                                        default:
                                                            check(0);
                                                            break;
                                                    }
                                            }

                                        binder.BindDescriptorSet(cmd, draw.Pipeline.PipelineLayout, descriptorSetLayout, set.first);
                                    }

                                // draw call
                                VkDeviceSize offset{ 0 };
                                vkCmdBindVertexBuffers(cmd, 0, 1, &draw.VertexBuffer, &offset);
                                vkCmdDraw(cmd, draw.VerticesCount, 1, draw.BeginVertex, 0);
                            }
                    }

                    // end renderpass
                    vkCmdEndRenderPass(cmd);
                }

                vkEndCommandBuffer(cmd);
            }
    }
    // end render pass

    // Queue submit
    auto recordedCommandBuffers = commandPool.GetRecorded();
    if (recordedCommandBuffers.size() > 0)
        {
            // Reset only when there is work submitted otherwise we'll wait indefinetly
            vkResetFences(VulkanDevice.Device, 1, &_fence[_frameIndex]);

            VulkanDevice.SubmitToMainQueue(recordedCommandBuffers, imageAvailableSemaphores, _workFinishedSemaphores[_frameIndex], _fence[_frameIndex]);
        }
    // Present
    if (recordedCommandBuffers.size() > 0 && _windowToSwapchain.size() > 0)
        {
            std::vector<VkResult> presentError = VulkanDevice.Present(swapchainImageIndex, { _workFinishedSemaphores[_frameIndex] });
            for (size_t i = 0; i < presentError.size(); i++)
                {
                    if (presentError[i] != VK_SUCCESS)
                        {
                            const VkSwapchainKHR swapchainInErrorState = swapchainImageIndex[i].first;
                            auto found = std::find_if(_windowToSwapchain.begin(), _windowToSwapchain.end(), [swapchainInErrorState](std::pair<const WindowIdentifier, SwapchainVk>& pair) {
                                return pair.second.Swapchain == swapchainInErrorState;
                            });

                            WaitGpuIdle();
                            _recreateSwapchain(found->second);
                            // throw std::runtime_error(VkErrorString(presentError[i]));
                        }
                }
        }

    // Clear render passes vector
    _renderPasses.clear();

    // Increment the frame index
    _frameIndex = (_frameIndex + 1) % GetNumberOfFramesInFlight();
}

void
RIContextVk::_waitFence(VkFence fence)
{
    const VkResult result = vkWaitForFences(VulkanDevice.Device, 1, &fence, VK_TRUE /*Wait all*/, MAX_FENCE_TIMEOUT);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkErrorString(result));
        }
}

void
RIContextVk::_recreateSwapchain(SwapchainVk& swapchain)
{
    // This functions expects that no swapchain resource is being used
    swapchain.Surface.SurfaceCapabilities = MT_GetSurfaceCapabilities(swapchain.Surface.Surface);

    std::for_each(swapchain.Framebuffers.begin(), swapchain.Framebuffers.end(), [this](VkFramebuffer fbo) { MT_DestroyFramebuffer(fbo); });
    swapchain.Framebuffers.clear();

    std::for_each(swapchain.ImageViews.begin(), swapchain.ImageViews.end(), [this](VkImageView view) { MT_DestroyImageView(view); });
    swapchain.ImageViews.clear();

    // Create from old
    {
        auto newSwapchain =
        VulkanDevice.CreateSwapchainFromSurface(swapchain.Surface.Surface, swapchain.SwapchainFormat, swapchain.SwapchainPresentMode, swapchain.Surface.SurfaceCapabilities, swapchain.Swapchain);

        MT_DestroySwapchain(swapchain.Swapchain);
        swapchain.Swapchain = newSwapchain;
    }

    MT_GetSwapchainImages(swapchain.Swapchain, swapchain.SwapchainImages);

    std::transform(swapchain.SwapchainImages.begin(), swapchain.SwapchainImages.end(), std::back_inserter(swapchain.ImageViews), [this, &swapchain](VkImage img) {
        return MT_CreateImageView(img, swapchain.SwapchainFormat.format);
    });

    DRenderPassAttachments att;
    att.Attachments.push_back(DRenderPassAttachment(
    swapchain.SwapchainFormat.format, ERenderPassLoad::Clear, ERenderPassStore::Store, ERenderPassLayout::Undefined, ERenderPassLayout::Present, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));

    VkRenderPass renderPass = MT_CreateRenderPass(att);

    std::transform(swapchain.ImageViews.begin(), swapchain.ImageViews.end(), std::back_inserter(swapchain.Framebuffers), [this, swapchain, renderPass](VkImageView view) {
        return MT_CreateFramebuffer({ view }, swapchain.Surface.SurfaceCapabilities.currentExtent, renderPass);
    });
}

#pragma endregion

}