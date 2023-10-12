// Copyright RedFox Studio 2022

#pragma once

#include "IContext.h"

#include "DescriptorPool.h"
#include "RingBufferManager.h"
#include "VulkanDevice13.h"
#include "VulkanInstance.h"

#include <array>
#include <functional>
#include <list>
#include <tuple>
#include <vector>

namespace Fox
{

struct DResource
{
    uint8_t Id{};
};

struct DBufferVulkan : public DResource
{
    uint32_t       Size{};
    RIVulkanBuffer Buffer;
};

struct DImageVulkan : public DResource
{
    std::string   DebugName{ "NoName" };
    RIVulkanImage Image;
    VkImageView   View{};
    VkSampler     Sampler{};
};

struct DFramebufferSingle
{
    VkFramebuffer              Framebuffer;
    std::vector<DImageVulkan*> ImageAttachments;
    std::vector<VkImageLayout> ImageLayouts;
};

struct DFramebufferVulkan : public DResource
{
    uint32_t Width{}, Height{};

    /*Per frame framebuffer if is a swapchain's framebuffer otherwise it must be only 1 framebuffer*/
    std::vector<DFramebufferSingle> Framebuffers;
    /*The attachments used to define a renderpass operation, by default is load-store OP*/
    DRenderPassAttachments Attachments;
    /*DEPRECATED*/
    RIVkRenderPassInfo RenderPassInfo;
    inline bool        IsSwapchainFramebuffer() const { return Framebuffers.size() > 1; };

    inline DFramebufferSingle& Get(uint32_t frameIndex) { return Framebuffers.at(frameIndex); };
};

struct DSwapchainVulkan : public DResource
{
    VkSurfaceKHR              Surface{};
    VkSurfaceCapabilitiesKHR  Capabilities;
    VkSurfaceFormatKHR        Format;
    VkPresentModeKHR          PresentMode;
    VkSwapchainKHR            Swapchain{};
    std::vector<DImageVulkan> Images;
    FramebufferId             Framebuffers{};
    std::vector<VkSemaphore>  ImageAvailableSemaphore;
    uint32_t                  CurrentImageIndex{};
};

struct DVertexInputLayoutVulkan : public DResource
{
    std::vector<VkVertexInputAttributeDescription> VertexInputAttributes;
};

struct DPipelineVulkan : public DPipeline_T
{
    VkPipeline           Pipeline{};
    VkPipelineLayout     PipelineLayout{};
    std::vector<EFormat> Attachments;
};

struct DPipelinePermutations
{
    std::unordered_map<PipelineFormat, VkPipeline, PipelineFormatHashFn, PipelineFormatEqualFn> Pipeline{};
};

/*This is a wrapper of a shader, which dynamically constructs the needed pipelines*/
struct DShaderVulkan : public DResource
{
    VertexInputLayoutId                          VertexLayout{};
    uint32_t                                     VertexStride{};
    VkShaderModule                               VertexShaderModule{};
    VkShaderModule                               PixelShaderModule{};
    std::vector<VkPipelineShaderStageCreateInfo> ShaderStageCreateInfo;
    uint32_t                                     ColorAttachments{ 1 };
    bool                                         DepthStencilAttachment{};
    /*Since VkDescriptorSetLayout are cached it can be used by multiple shaders, be careful when deleting*/
    std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;
    /*Since pipeline layouts are cached it can be used by multiple shaders, be careful when deleting*/
    VkPipelineLayout PipelineLayout{};

    // Hash to pipeline map
    std::unordered_map<std::vector<DRenderPassAttachment>, DPipelinePermutations, DRenderPassAttachmentsFormatsOnlyHashFn, DRenderPassAttachmentsFormatsOnlyEqualFn>
    RenderPassFormatToPipelinePermutationMap;
};

struct FramegraphImage : public FramegraphResource
{
    const DImageVulkan* Image;

    FramegraphImage(const DImageVulkan* image, std::string debugName, std::string description) : FramegraphResource(EResourceType::IMAGE, debugName, description), Image(image){};
    VkImageLayout CurrenLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;

    void TransitionRW(VkCommandBuffer cmd, VkImageLayout destination)
    {
        VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, NULL };
        barrier.srcAccessMask                   = VK_ACCESS_MEMORY_READ_BIT;
        barrier.dstAccessMask                   = VK_ACCESS_MEMORY_WRITE_BIT;
        barrier.oldLayout                       = CurrenLayout;
        barrier.newLayout                       = destination;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                           = Image->Image.Image;
        barrier.subresourceRange.aspectMask     = VkUtils::isColorFormat(Image->Image.Format) ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = Image->Image.MipLevels;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;

        CurrenLayout = destination;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, (uint32_t)1, &barrier);
    }
    void TransitionWR(VkCommandBuffer cmd, VkImageLayout destination)
    {
        VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, NULL };
        barrier.srcAccessMask                   = VK_ACCESS_MEMORY_WRITE_BIT;
        barrier.dstAccessMask                   = VK_ACCESS_MEMORY_READ_BIT;
        barrier.oldLayout                       = CurrenLayout;
        barrier.newLayout                       = destination;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                           = Image->Image.Image;
        barrier.subresourceRange.aspectMask     = VkUtils::isColorFormat(Image->Image.Format) ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = Image->Image.MipLevels;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;

        CurrenLayout = destination;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 0, nullptr, (uint32_t)1, &barrier);
    }
};

struct FramegraphVertexBuffer : public FramegraphResource
{
    const DBufferVulkan* Buffer;
    FramegraphVertexBuffer(const DBufferVulkan* buffer, std::string debugName, std::string description)
      : FramegraphResource(EResourceType::VERTEX_INDEX_BUFFER, debugName, description), Buffer(buffer){};

    void WaitReadToWriteTransferBarrier(VkCommandBuffer cmd)
    {
        VkBufferMemoryBarrier barrier{};
        barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.pNext               = NULL;
        barrier.srcAccessMask       = VK_ACCESS_MEMORY_READ_BIT;
        barrier.dstAccessMask       = VK_ACCESS_MEMORY_WRITE_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer              = Buffer->Buffer.Buffer;
        barrier.offset              = 0;
        barrier.size                = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, NULL, 0, NULL, 1, &barrier, 0, NULL);
    };

    void WaitWriteToReadTransferBarrier(VkCommandBuffer cmd)
    {
        VkBufferMemoryBarrier barrier{};
        barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.pNext               = NULL;
        barrier.srcAccessMask       = VK_ACCESS_MEMORY_WRITE_BIT;
        barrier.dstAccessMask       = VK_ACCESS_MEMORY_READ_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer              = Buffer->Buffer.Buffer;
        barrier.offset              = 0;
        barrier.size                = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, NULL, 0, NULL, 1, &barrier, 0, NULL);
    };
};

struct FramegraphUbo : public FramegraphResource
{
    const DBufferVulkan* Buffer;
    FramegraphUbo(const DBufferVulkan* buffer, std::string debugName, std::string description) : FramegraphResource(EResourceType::UNIFORM_BUFFER, debugName, description), Buffer(buffer){};
    void WaitReadToWriteTransferBarrier(VkCommandBuffer cmd)
    {
        VkBufferMemoryBarrier barrier{};
        barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.pNext               = NULL;
        barrier.srcAccessMask       = VK_ACCESS_MEMORY_READ_BIT;
        barrier.dstAccessMask       = VK_ACCESS_MEMORY_WRITE_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer              = Buffer->Buffer.Buffer;
        barrier.offset              = 0;
        barrier.size                = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, NULL, 0, NULL, 1, &barrier, 0, NULL);
    };

    void WaitWriteToReadTransferBarrier(VkCommandBuffer cmd)
    {
        VkBufferMemoryBarrier barrier{};
        barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.pNext               = NULL;
        barrier.srcAccessMask       = VK_ACCESS_MEMORY_WRITE_BIT;
        barrier.dstAccessMask       = VK_ACCESS_MEMORY_READ_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer              = Buffer->Buffer.Buffer;
        barrier.offset              = 0;
        barrier.size                = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, NULL, 0, NULL, 1, &barrier, 0, NULL);
    };
};

struct CommandBarrier : public CommandBase
{
    CommandBarrier() : CommandBase(ECommandType::BARRIER)
    {
        barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.pNext               = NULL;
        barrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask       = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.offset              = 0;
        barrier.size                = VK_WHOLE_SIZE;
    };

    CommandBarrier& SetBuffer(VkBuffer buffer)
    {
        barrier.buffer = buffer;
        return *this;
    };

    CommandBarrier& SrcAccess(VkAccessFlags accessFlag)
    {
        barrier.srcAccessMask = accessFlag;
        return *this;
    };
    CommandBarrier& DstAccess(VkAccessFlags dstAccessMask)
    {
        barrier.dstAccessMask = dstAccessMask;
        return *this;
    };

    VkBufferMemoryBarrier barrier{};
};

struct CommandImageTransition : public CommandBase
{
    CommandImageTransition() : CommandBase(ECommandType::IMAGE_TRANSITION)
    {
        barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1; // number of mip maps
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;
    };

    CommandImageTransition& SetColorBit()
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        return *this;
    };
    CommandImageTransition& DstAccess(VkImage image)
    {
        barrier.image = image;
        return *this;
    };

    CommandImageTransition& From(VkImageLayout oldLayout)
    {
        barrier.oldLayout = oldLayout;
        return *this;
    };
    CommandImageTransition& To(VkImageLayout newLayout)
    {
        barrier.newLayout = newLayout;
        return *this;
    };
    CommandImageTransition& SrcAccess(VkAccessFlags accessFlag)
    {
        barrier.srcAccessMask = accessFlag;
        return *this;
    };
    CommandImageTransition& DstAccess(VkAccessFlags dstAccessMask)
    {
        barrier.dstAccessMask = dstAccessMask;
        return *this;
    };
    CommandImageTransition& MipsRange(uint32_t mipMapBase, uint32_t mipMapCount)
    {
        barrier.subresourceRange.baseMipLevel = mipMapBase;
        barrier.subresourceRange.levelCount   = mipMapCount; // number of mip maps
        return *this;
    };

    VkImageMemoryBarrier barrier{};
};

// template<class Node>
// class Graph
//{
//   public:
//     Graph() = default;
//
//     std::vector<std::pair<Node, size_t>> Graph;
// };

class VulkanContext final : public IContext
{
    inline static constexpr uint32_t NUM_OF_FRAMES_IN_FLIGHT{ 2 };

  public:
    VulkanContext(const DContextConfig* const config);
    ~VulkanContext();

    SwapchainId CreateSwapchain(const WindowData* windowData, EPresentMode& presentMode, EFormat& outFormat) override;
    void        DestroySwapchain(SwapchainId swapchainId) override;

    FramebufferId CreateSwapchainFramebuffer(SwapchainId swapchainId) override;
    void          DestroyFramebuffer(FramebufferId framebufferId) override;

    BufferId            CreateVertexBuffer(uint32_t size) override;
    BufferId            CreateUniformBuffer(uint32_t size) override;
    void                DestroyBuffer(BufferId buffer) override;
    ImageId             CreateImage(EFormat format, uint32_t width, uint32_t height, uint32_t mipMapCount, std::string debugName) override;
    void                DestroyImage(ImageId imageId) override;
    VertexInputLayoutId CreateVertexLayout(const std::vector<VertexLayoutInfo>& info) override;
    ShaderId            CreateShader(const ShaderSource& source) override;
    void                DestroyShader(const ShaderId shader) override;

    void     SubmitPass(RenderPassData&& data) override;
    void     SubmitCopy(CopyDataCommand&& data) override;
    uint32_t SubmitCommand(std::unique_ptr<CommandBase>&& command) override;
    void     AdvanceFrame() override;
    void     FlushDeletedBuffers() override;
    void     ExportGraphviz(const std::string filename) override;

    unsigned char* GetAdapterDescription() const override;
    size_t         GetAdapterDedicatedVideoMemory() const override;

#pragma region Utility
    void              WaitDeviceIdle();
    RIVulkanDevice13& GetDevice() { return Device; }
#pragma endregion

  private:
    static constexpr uint32_t MAX_RESOURCES     = 4096;
    static constexpr uint64_t MAX_FENCE_TIMEOUT = 0xffff; // 0xffffffffffffffff; // nanoseconds
    RIVulkanInstance          Instance;
    RIVulkanDevice13          Device;

    void (*_warningOutput)(const char*);
    void (*_logOutput)(const char*);

    std::array<DSwapchainVulkan, MAX_RESOURCES>         _swapchains;
    std::array<DBufferVulkan, MAX_RESOURCES>            _vertexBuffers;
    std::array<DBufferVulkan, MAX_RESOURCES>            _uniformBuffers;
    std::array<DFramebufferVulkan, MAX_RESOURCES>       _framebuffers;
    std::array<DShaderVulkan, MAX_RESOURCES>            _shaders;
    std::array<DVertexInputLayoutVulkan, MAX_RESOURCES> _vertexLayouts;
    std::array<DImageVulkan, MAX_RESOURCES>             _images;
    std::vector<std::unique_ptr<CommandBase>>           _commands;
    std::vector<std::unique_ptr<CommandBase>>           _compiledCommands;
    std::vector<std::unique_ptr<FramegraphResource>>    _framegraphResources;

    std::unordered_set<VkRenderPass> _renderPasses;
    using DeleteFn                 = std::function<void()>;
    using FramesWaitToDeletionList = std::pair<uint32_t, std::vector<DeleteFn>>;
    uint32_t                                                                   _frameIndex{};
    std::vector<FramesWaitToDeletionList>                                      _deletionQueue;
    std::array<std::unique_ptr<RICommandPoolManager>, NUM_OF_FRAMES_IN_FLIGHT> _cmdPool;
    std::array<VkSemaphore, NUM_OF_FRAMES_IN_FLIGHT>                           _workFinishedSemaphores;
    std::array<VkFence, NUM_OF_FRAMES_IN_FLIGHT>                               _fence;
    std::vector<CopyDataCommand>                                               _transferCommands;
    std::vector<RenderPassData>                                                _drawCommands;
    // Staging buffer, used to copy stuff from ram to CPU-VISIBLE-MEMORY then to GPU-ONLY-MEMORY
    std::vector<std::vector<uint32_t>> _perFrameCopySizes;
    RIVulkanBuffer                     _stagingBuffer;
    std::unique_ptr<RingBufferManager> _stagingBufferManager;

    // Pipeline layout to map of descriptor set layout - used to retrieve the correct VkDescriptorSetLayout when creating descriptor set
    std::unordered_map<VkPipelineLayout, std::map<uint32_t, VkDescriptorSetLayout>> _pipelineLayoutToSetIndexDescriptorSetLayout;
    /*Per frame map of per pipeline layout to descriptor pool, we can allocate descriptor sets per pipeline layout*/
    std::vector<std::unordered_map<VkPipelineLayout, std::shared_ptr<RIDescriptorPoolManager>>> _pipelineLayoutToDescriptorPool;

    const std::vector<const char*> _validationLayers = {
        "VK_LAYER_KHRONOS_validation",
        "VK_LAYER_GOOGLE_threading",
        "VK_LAYER_LUNARG_parameter_validation",
        "VK_LAYER_LUNARG_object_tracker",
        "VK_LAYER_LUNARG_core_validation",
        "VK_LAYER_GOOGLE_unique_objects",
    };

#if FOX_PLATFORM == FOX_PLATFORM_WINDOWS32
    const std::vector<const char*> _instanceExtensionNames = { "VK_EXT_debug_utils",
        "VK_KHR_surface",
        "VK_KHR_win32_surface",
        "VK_KHR_external_semaphore_capabilities",
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME };
#elif FOX_PLATFORM == FOX_PLATFORM_LINUX
    const std::vector<const char*> _instanceExtensionNames = {
        "VK_EXT_debug_utils",
        "VK_KHR_surface",
        "VK_KHR_wayland_surface",
        "VK_KHR_xlib_surface",
        "VK_KHR_external_semaphore_capabilities",
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
    };
#else
#pragma error "Not supported"
#endif

    const std::vector<const char*> _deviceExtensionNames = { VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
        "VK_KHR_Maintenance1", // passing negative viewport heights
        "VK_KHR_maintenance4",
        "VK_KHR_dedicated_allocation",
        "VK_KHR_bind_memory2" };

    void Warning(const std::string& error);
    void Log(const std::string& error);

    void _initializeVolk();
    void _initializeInstance();
    void _initializeDebugger();
    void _initializeVersion();
    void _initializeDevice();
    void _initializeStagingBuffer(uint32_t stagingBufferSize);
    void _deinitializeStagingBuffer();

    void               _createSwapchain(DSwapchainVulkan& swapchain, const WindowData* windowData, EPresentMode& presentMode, EFormat& outFormat);
    void               _createVertexBuffer(uint32_t size, DBufferVulkan& buffer);
    void               _createUniformBuffer(uint32_t size, DBufferVulkan& buffer);
    void               _createShader(const ShaderSource& source, DShaderVulkan& shader);
    void               _createFramebufferFromSwapchain(DSwapchainVulkan& swapchain);
    void               _performDeletionQueue();
    void               _performCopyOperations();
    void               _submitCommands(const std::vector<VkSemaphore>& imageAvailableSemaphores);
    void               _deferDestruction(DeleteFn&& fn);
    VkPipeline         _createPipeline(VkPipelineLayout         pipelineLayout,
            VkRenderPass                                        renderPass,
            const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
            const PipelineFormat&                               format,
            const DVertexInputLayoutVulkan&                     vertexLayout,
            uint32_t                                            stride);
    void               _recreateSwapchainBlocking(DSwapchainVulkan& swapchain);
    RIVkRenderPassInfo _computeFramebufferAttachmentsRenderPassInfo(const std::vector<VkFormat>& attachmentFormat);
    VkPipeline         _queryPipelineFromAttachmentsAndFormat(DShaderVulkan& shader, const DRenderPassAttachments& renderPass, const PipelineFormat& format);
    void               _executeCopyCommand(VkCommandBuffer cmd, const std::unique_ptr<CommandBase>& ptr);

    static VKAPI_ATTR VkBool32 VKAPI_CALL _vulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT                                                                   messageType,
    const VkDebugUtilsMessengerCallbackDataEXT*                                                       pCallbackData,
    void*                                                                                             pUserData);
    VkRenderPass                          _createRenderPass(const DRenderPassAttachments& attachments);
    VkRenderPass                          _createRenderPassFromInfo(const RIVkRenderPassInfo& info);
    std::vector<const char*>              _getInstanceSupportedExtensions(const std::vector<const char*>& extentions);
    std::vector<const char*>              _getInstanceSupportedValidationLayers(const std::vector<const char*>& validationLayers);
    VkPhysicalDevice                      _queryBestPhysicalDevice();
    std::vector<const char*>              _getDeviceSupportedExtensions(VkPhysicalDevice physicalDevice, const std::vector<const char*>& extentions);
    std::vector<const char*>              _getDeviceSupportedValidationLayers(VkPhysicalDevice physicalDevice, const std::vector<const char*>& validationLayers);

#pragma region Framegraph functions
    void _compileGraph();
    /*Adds an image to the resource vector or returns if already exists*/
    FramegraphImage* _makeFramegraphImage(ImageId id);
    FramegraphImage* _makeFramegraphImagePtr(const DImageVulkan* imagePtr);
    /*Adds an UBO to the resource vector or returns if already exists*/
    FramegraphUbo* _makeFramegraphUbo(BufferId id);
    FramegraphUbo* _makeFramegraphUboPtr(const DBufferVulkan* ptr);
    /*Adds an Vertex buffer to the resource vector or returns if already exists*/
    FramegraphVertexBuffer* _makeFramegraphVertexBuffer(BufferId id);
    FramegraphVertexBuffer* _makeFramegraphVertexBufferPtr(const DBufferVulkan* ptr);
#pragma endregion
};

RIVkRenderPassInfo ConvertRenderPassAttachmentsToRIVkRenderPassInfo(const DRenderPassAttachments& attachments);
}