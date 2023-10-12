// Copyright RedFox Studio 2022

#pragma once

#if defined(FOX_USE_VULKAN)

#include "RICoreVk.h"

#include "Core/memory/RingBufferManager.h"
#include "Core/window/WindowBase.h"
#include "DescriptorPool.h"
#include "UtilsVK.h"

#include <array>
#include <cstdlib>

namespace Fox
{
struct DRIBuffer
{
    VkBuffer      Buffer{};
    VmaAllocation Allocation{};
};

enum class ERenderPassLoad
{
    Load,
    Clear,
};
enum class ERenderPassStore
{
    Store,
    DontCare
};

enum class ERenderPassLayout
{
    Undefined,
    AsAttachment,
    ShaderReadOnly,
    Present
};

struct DRenderPassAttachment
{
    VkFormat              Format;
    VkSampleCountFlagBits Samples{ VK_SAMPLE_COUNT_1_BIT };
    ERenderPassLoad       LoadOP;
    ERenderPassStore      StoreOP;
    ERenderPassLayout     InitialLayout;
    ERenderPassLayout     FinalLayout;
    VkImageLayout         AttachmentReferenceLayout;

    DRenderPassAttachment(VkFormat format, ERenderPassLoad loadOp, ERenderPassStore storeOp, ERenderPassLayout initLayout, ERenderPassLayout finalLayout)
      : Format(format), LoadOP(loadOp), StoreOP(storeOp), InitialLayout(initLayout), FinalLayout(finalLayout)
    {
        AttachmentReferenceLayout = format == VK_FORMAT_B8G8R8A8_SRGB ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    };

    DRenderPassAttachment(VkFormat format, ERenderPassLoad loadOp, ERenderPassStore storeOp, ERenderPassLayout initLayout, ERenderPassLayout finalLayout, VkImageLayout attachmentReferenceLayout)
      : Format(format), LoadOP(loadOp), StoreOP(storeOp), InitialLayout(initLayout), FinalLayout(finalLayout), AttachmentReferenceLayout(attachmentReferenceLayout){};

    bool operator==(DRenderPassAttachment const& o) const
    {
        return Format == o.Format && Samples == o.Samples && LoadOP == o.LoadOP && StoreOP == o.StoreOP && InitialLayout == o.InitialLayout && FinalLayout == o.FinalLayout &&
        AttachmentReferenceLayout == o.AttachmentReferenceLayout;
    };

    inline VkFormat              GetFormat() const { return Format; };
    inline VkSampleCountFlagBits GetSamplesCount() const { return Samples; };
    inline VkAttachmentLoadOp    GetLoadOp() const
    {
        switch (LoadOP)
            {
                case ERenderPassLoad::Load:
                    return VK_ATTACHMENT_LOAD_OP_LOAD;
                    break;
                case ERenderPassLoad::Clear:
                    return VK_ATTACHMENT_LOAD_OP_CLEAR;
                    break;
            }
        check(0);
        return VK_ATTACHMENT_LOAD_OP_CLEAR;
    };
    inline VkAttachmentStoreOp GetStoreOp() const
    {
        switch (StoreOP)
            {
                case ERenderPassStore::Store:
                    return VK_ATTACHMENT_STORE_OP_STORE;
                    break;
                case ERenderPassStore::DontCare:
                    return VK_ATTACHMENT_STORE_OP_DONT_CARE;
                    break;
            }
        check(0);
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    };
    inline VkImageLayout GetInitialLayout() const
    {
        switch (InitialLayout)
            {
                case ERenderPassLayout::Undefined:
                    return VK_IMAGE_LAYOUT_UNDEFINED;
                case ERenderPassLayout::AsAttachment:
                    return Format == VK_FORMAT_D32_SFLOAT_S8_UINT ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                case ERenderPassLayout::ShaderReadOnly:
                    return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                case ERenderPassLayout::Present:
                    return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            }
        check(0);
        return VK_IMAGE_LAYOUT_UNDEFINED;
    };
    inline VkImageLayout GetFinalLayout() const
    {
        switch (FinalLayout)
            {
                case ERenderPassLayout::Undefined:
                    return VK_IMAGE_LAYOUT_UNDEFINED;
                case ERenderPassLayout::AsAttachment:
                    return Format == VK_FORMAT_D32_SFLOAT_S8_UINT ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                case ERenderPassLayout::ShaderReadOnly:
                    return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                case ERenderPassLayout::Present:
                    return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            }
        check(0);
        return VK_IMAGE_LAYOUT_UNDEFINED;
    };
    inline VkImageLayout GetAttachmentReferenceLayout() const { return AttachmentReferenceLayout; };
    inline size_t        Hash() const
    {

        size_t hash = 0;

        hash += (71 * hash + Format) % 5;
        hash += (71 * hash + (size_t)LoadOP) % 5;
        hash += (71 * hash + (size_t)StoreOP) % 5;
        hash += (71 * hash + (size_t)InitialLayout) % 5;
        hash += (71 * hash + (size_t)FinalLayout) % 5;
        hash += (71 * hash + (size_t)AttachmentReferenceLayout) % 5;

        return hash;
    };

    static inline DRenderPassAttachment MakeClearToPresent(const VkFormat format)
    {
        return DRenderPassAttachment(format, ERenderPassLoad::Clear, ERenderPassStore::Store, ERenderPassLayout::Undefined, ERenderPassLayout::Present);
    };
    static inline DRenderPassAttachment MakeClearToAttachment(const VkFormat format)
    {
        return DRenderPassAttachment(format, ERenderPassLoad::Clear, ERenderPassStore::Store, ERenderPassLayout::Undefined, ERenderPassLayout::AsAttachment);
    };
    static inline DRenderPassAttachment MakeLoadNoStoreToAttachment(const VkFormat format)
    {
        return DRenderPassAttachment(format, ERenderPassLoad::Load, ERenderPassStore::DontCare, ERenderPassLayout::Undefined, ERenderPassLayout::AsAttachment);
    };
};

struct DRenderPassAttachments
{
    std::vector<DRenderPassAttachment> Attachments;
};

class DRenderPassAttachmentsHasher
{
  public:
    size_t operator()(DRenderPassAttachments const& key) const
    {
        size_t hash = 0;
        for (size_t i = 0; i < key.Attachments.size(); i++)
            {
                hash += key.Attachments[i].Hash();
            }
        return hash;
    }
};
class DRenderPassAttachmentsEqualFn
{
  public:
    bool operator()(DRenderPassAttachments const& t1, DRenderPassAttachments const& t2) const { return t1.Attachments == t2.Attachments; }
};

struct SetBuffer
{
    VkBuffer Buffer{};
    uint32_t Offset{};
    uint32_t Range{};
};

struct SetImage
{
    VkImageView   ImageView{};
    VkImageLayout ImageLayout{};
    VkSampler     Sampler{};
};

struct SetBinding
{
    /* Only a buffer or image is a valid at once*/
    VkDescriptorType       Type{};
    std::vector<SetBuffer> Buffers;
    std::vector<SetImage>  Images;
};

struct PipelineVk
{
    VkPipeline       Pipeline;
    VkPipelineLayout PipelineLayout;
};

struct VulkanUniformBuffer
{
    RIVulkanBuffer Buffer{};
    uint32_t       Size{};
};

struct DrawCommand
{
    // Ctor
    PipelineVk Pipeline;
    // Bind... functions
    std::map<uint32_t /*set*/, std::map<uint32_t /*binding*/, SetBinding>> DescriptorSetBindings;
    // Draw... functions
    VkBuffer VertexBuffer{};
    VkBuffer IndexBuffer{}; // optional
    uint32_t BeginVertex{};
    uint32_t VerticesCount{};

    DrawCommand(PipelineVk pipeline) : Pipeline(pipeline) {}

    inline void BindBufferUniformBuffer(uint32_t set, uint32_t bindingIndex, VulkanUniformBuffer* buffer)
    {
        constexpr uint32_t offset = 0;
        SetBuffer          bufferInfo{ buffer->Buffer.Buffer, offset, buffer->Size };
        SetBinding         binding{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, { bufferInfo }, {} };
        DescriptorSetBindings[set][bindingIndex] = (std::move(binding));
    }

    inline void BindImageArray(uint32_t set, uint32_t bindingIndex, const std::vector<std::pair<VkImageView, VkSampler>>& imageToSamplerArray)
    {
        std::vector<SetImage> images(imageToSamplerArray.size());
        std::transform(imageToSamplerArray.begin(), imageToSamplerArray.end(), std::back_inserter(images), [](const std::pair<VkImageView, VkSampler>& p) {
            return SetImage{ p.first, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, p.second };
        });

        SetBinding binding{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, {}, images };
        DescriptorSetBindings[set][bindingIndex] = (std::move(binding));
    }

    inline void Draw(VkBuffer vertexBuffer, uint32_t start, uint32_t count)
    {
        VertexBuffer  = vertexBuffer;
        BeginVertex   = start;
        VerticesCount = count;
    }
};

struct WindowSurface
{
    /*Window Surface*/
    VkSurfaceKHR                    Surface{};
    std::vector<VkSurfaceFormatKHR> SurfaceFormatsSupported;
    std::vector<VkPresentModeKHR>   SurfacePresentModesSupported;
    VkSurfaceCapabilitiesKHR        SurfaceCapabilities;
};

struct SwapchainVk
{
    WindowSurface        Surface;
    VkSwapchainKHR       Swapchain{};
    VkSurfaceFormatKHR   SwapchainFormat{ VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_MAX_ENUM_KHR };
    VkPresentModeKHR     SwapchainPresentMode{ VK_PRESENT_MODE_MAX_ENUM_KHR };
    std::vector<VkImage> SwapchainImages;
    /*Imageviews*/
    std::vector<VkImageView> ImageViews;
    /*image index used to index the framebuffer*/
    uint32_t CurrentImageIndex{ UINT32_MAX };
    /*Swapchain image is the only attachment, number if defined by the number of SwapchainImages*/
    std::vector<VkFramebuffer> Framebuffers;

    std::vector<VkSemaphore> ImageAvailableSemaphore{}; // per frame semaphore
};

struct FramebufferRef
{
    SwapchainVk* Swapchain;
};

struct RenderPassData
{
    FramebufferRef            Framebuffer{};
    VkViewport                Viewport;
    VkRenderPass              RenderPass;
    std::vector<VkClearValue> ClearValues_DEPRECATED; // Equal to the RenderPass attachments with clear op
    std::vector<DrawCommand>  DrawCommands;

    RenderPassData(FramebufferRef fbo, VkViewport viewport, VkRenderPass renderPass) : Framebuffer(fbo), Viewport(viewport), RenderPass(renderPass) {}
    inline void ClearColor(float r, float g, float b, float a = 1.f)
    {
        VkClearColorValue col{ r, g, b, a };
        ClearValues_DEPRECATED.push_back({ col });
    }
    inline void ClearDepthStencil(float depth, uint32_t stencil)
    {
        VkClearValue clearValue;
        clearValue.depthStencil = { depth, stencil };
        ClearValues_DEPRECATED.push_back(clearValue);
    }

    inline void AddDrawCommand(DrawCommand&& command) { DrawCommands.emplace_back(std::move(command)); }
};

struct CopyMipMapLevel
{
    uint32_t                   MipLevel{};
    uint32_t                   Width, Height;
    uint32_t                   Offset{}; // The offset in the destination VkImage buffer
    std::vector<unsigned char> Data;
};

struct CopyImageCommand
{
    VkImage                      Destination{};
    std::vector<CopyMipMapLevel> MipMapCopy;
};

struct CopyVertexCommand
{
    VkBuffer                   Destination{};
    uint32_t                   DestOffset{};
    std::vector<unsigned char> Data;
};

struct CopyUniformBufferCommand
{
    VulkanUniformBuffer*       Destination{};
    uint32_t                   DestOffset{};
    std::vector<unsigned char> Data;
};

struct CopyDataCommand
{
    std::optional<CopyVertexCommand>        VertexCommand;
    std::optional<CopyImageCommand>         ImageCommand;
    std::optional<CopyUniformBufferCommand> UniformCommand;

    inline void CopyVertex(VkBuffer destination, uint32_t destinationOffset, void* data, uint32_t bytes)
    {
        check(!VertexCommand.has_value());
        check(!ImageCommand.has_value());
        check(!UniformCommand.has_value());

        std::vector<unsigned char> blob(bytes, 0);
        memcpy(blob.data(), data, bytes);
        VertexCommand = CopyVertexCommand{ destination, destinationOffset, std::move(blob) };
    };

    inline void CopyUniformBuffer(VulkanUniformBuffer* ubo, void* data, uint32_t bytes)
    {
        check(!VertexCommand.has_value());
        check(!ImageCommand.has_value());
        check(!UniformCommand.has_value());

        std::vector<unsigned char> blob(bytes, 0);
        memcpy(blob.data(), data, bytes);
        UniformCommand = CopyUniformBufferCommand{ ubo, 0, std::move(blob) };
    }

    inline void CopyImageMipMap(VkImage destination, uint32_t offset, void* data, uint32_t width, uint32_t height, uint32_t mipMapIndex, uint32_t bytes)
    {
        check(!VertexCommand.has_value());
        check(!ImageCommand.has_value());
        check(!UniformCommand.has_value());

        std::vector<unsigned char> blob(bytes, 0);
        memcpy(blob.data(), data, bytes);

        std::vector<CopyMipMapLevel> level;
        CopyMipMapLevel              l;
        l.Data     = std::move(blob);
        l.MipLevel = mipMapIndex;
        l.Width    = width;
        l.Height   = height;
        l.Offset   = offset;

        level.push_back(std::move(l));
        ImageCommand = CopyImageCommand{ destination, level };
    }
};

class RIFactoryVk : public RICoreVk
{
  public:
    RIFactoryVk();
    virtual ~RIFactoryVk();

#pragma region Surface
    VkSurfaceKHR                    MT_CreateSurfaceFromWindow(AWindow* window);
    void                            MT_DestroySurface(VkSurfaceKHR surface);
    bool                            MT_SurfaceSupportPresentationOnCurrentQueueFamily(VkSurfaceKHR surface);
    std::vector<VkSurfaceFormatKHR> MT_GetSurfaceFormats(VkSurfaceKHR surface);
    VkSurfaceCapabilitiesKHR        MT_GetSurfaceCapabilities(VkSurfaceKHR surface);
    std::vector<VkPresentModeKHR>   MT_GetSurfacePresentModes(VkSurfaceKHR surface);
#pragma endregion

#pragma region Swapchain
    VkSwapchainKHR         MT_CreateSwapchainFromSurface(VkSurfaceKHR surface, VkSurfaceFormatKHR& format, VkPresentModeKHR& presentMode);
    void                   MT_DestroySwapchain(VkSwapchainKHR swapchain);
    VkSwapchainKHR         MT_RecreateSwapchainFromOld(VkSurfaceKHR surface, VkSwapchainKHR oldSwapchain, VkSurfaceFormatKHR format, VkPresentModeKHR presentMode);
    void                   MT_GetSwapchainImages(VkSwapchainKHR swapchain, std::vector<VkImage>& images);
    ERI_ERROR              MT_TryAcquireNextImage(VkSwapchainKHR swapchain, uint32_t* imageIndex, VkSemaphore imageAvailable);
    std::vector<ERI_ERROR> MT_Present(std::vector<std::pair<VkSwapchainKHR, uint32_t>> swapchainImageIndex, std::vector<VkSemaphore> waitSemaphores);
#pragma endregion

#pragma region Synchronization primitives
    VkSemaphore MT_CreateSemaphore();
    void        MT_DestroySemaphore(VkSemaphore semaphore);

    RIFence* CreateFence(bool signaled);
    void     DestroyFence(RIFence* fence);
#pragma endregion

#pragma region Buffers
    VkBuffer MT_CreateMappedBufferHostVisibile(size_t size, VkBufferUsageFlagBits usage, VmaAllocation& allocation);
    VkBuffer MT_CreateMappedBufferGpuOnly(size_t size, VkBufferUsageFlagBits usage, VmaAllocation& allocation);
    void     MT_DestroyBuffer(VkBuffer buffer, VmaAllocation allocation);
    void     MT_CopyDataToMemory(VmaAllocation allocation, void* data, size_t bytes, size_t offset = 0);
    void*    MapMemory(VmaAllocation allocation);
    void     UnmapMemory(VmaAllocation allocation);
#pragma endregion

#pragma region Images

    RIVulkanImage MT_CreateImageGpuOnly2(int width, int height, int mipLevels, EImageFormat format, VkImageUsageFlags usage);
    VkImage       MT_CreateImageGpuOnly(int width, int height, int mipLevels, EImageFormat format, VkImageUsageFlags usage, VmaAllocation& allocation);
    VkImageView   MT_CreateImageView(VkImage image, VkFormat format);
    VkImageView   MT_CreateImageView(EImageFormat format, VkImage image, VkImageAspectFlags aspect);
    void          MT_DestroyImageView(VkImageView imageView);
    void          MT_DestroyImage(VkImage image, VmaAllocation allocation);
    void          MT_DestroyImage2(RIVulkanImage& image);
#pragma endregion

#pragma region Textures

    VkBuffer  MT_CreateTextureSampler2D(size_t size);
    VkSampler MT_CreateSampler(int minLod = 0, int maxLod = 0);
    void      MT_DestroySampler(VkSampler sampler);
#pragma endregion

#pragma region Framebuffer
    VkFramebuffer MT_CreateFramebuffer(const std::vector<VkImageView> imageViews, VkExtent2D extent, VkRenderPass renderPass);
    void          MT_DestroyFramebuffer(VkFramebuffer framebuffer);
#pragma endregion

#pragma region Shaders
    //@TODO
    std::vector<VkPipelineShaderStageCreateInfo> MT_CreateShaderStages(const RIShaderByteCode& bytecode);
    void                                         MT_DestroyShaderStages(const std::vector<VkPipelineShaderStageCreateInfo>& stages);
#pragma endregion

#pragma region DescriptorSetLayout
    VkDescriptorSetLayout MT_CreateDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo layoutCreateInfo);
    VkDescriptorSetLayout MT_CreateDescriptorSetLayout(const std::vector<RIShaderDescriptorBindings>& bindings);
    void                  MT_DestroyDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout);
#pragma endregion

#pragma region PipelineLayout
    VkPipelineLayout MT_CreateWorldPipelineLayout_DEPRECATED(const std::vector<VkDescriptorSetLayout>& descriptorSetLayout, const std::vector<VkPushConstantRange>& pushConstantRange);
    void             MT_DestroyPipelineLayout_DEPRECATED(VkPipelineLayout pipelineLayout);
#pragma endregion

#pragma region Pipeline
    VkPipeline MT_CreateWorldPipeline(VkPipelineLayout  pipelineLayout,
    VkRenderPass                                        renderPass,
    const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
    std::vector<RIInputSlot>                            vertexInput,
    uint32_t                                            vertexStrideBytes);
    VkPipeline MT_CreateWorldDebugPipeline(VkPipelineLayout pipelineLayout,
    VkRenderPass                                            renderPass,
    const std::vector<VkPipelineShaderStageCreateInfo>&     shaderStages,
    std::vector<RIInputSlot>                                vertexInput,
    uint32_t                                                vertexStrideBytes);

    VkPipeline MT_CreatePipeline(VkPipelineLayout pipelineLayout, VkRenderPass renderPass, const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages, const DRIPipelineFormat& format);

    PipelineVk CreatePipeline(const RIShaderSource& shader, const DRIPipelineFormat& format);

    void MT_DestroyPipeline(VkPipeline pipeline);

#pragma endregion

#pragma region Descriptor Set
    VkDescriptorSetLayout        MT_CreateWorldDescriptorSetLayout();
    VkDescriptorSetLayout        MT_CreatePostProcessDescriptorSetLayout();
    VkDescriptorSetLayout        MT_CreateWorldMaterialDescriptorSetLayout();
    std::vector<VkDescriptorSet> MT_CreateDescriptorSet(VkDescriptorPool descriptorPool, const std::vector<VkDescriptorSetLayout>& descriptorSetLayout);
    VkDescriptorPool             MT_CreateDescriptorPool(const std::unordered_map<VkDescriptorType, uint32_t>& typeToCountMap, int maxSets);
    void                         MT_DestroyDescriptorPool(VkDescriptorPool descriptorPool);
    void                         MT_UpdateWorldCameraDescriptorSet(VkDescriptorSet descriptorSet, VkBuffer buffer, size_t bytes, uint32_t binding, size_t offset = 0);
    void                         MT_UpdateDescriptorSet(const std::vector<VkWriteDescriptorSet>& writes);

    RIDescriptorSetBinder                    CreateDescriptorSetBinder();
    std::unique_ptr<RIDescriptorPoolManager> CreateDescriptorPoolManager(VkDescriptorPool pool, uint32_t maxSets);
#pragma endregion
    RICommandPool* MT_CreateCommandPool2();
    void           MT_DestroyCommandPool2(RICommandPool* pool);
    void           MT_SubmitQueue2(const std::vector<RICommandBuffer*>& cmds, const std::vector<VkSemaphore>& waitSemaphore, VkSemaphore finishSemaphore, RIFence* fence);

    VkCommandPool                MT_CreateCommandPool();
    void                         MT_DestroyCommandPool(VkCommandPool commandPool);
    std::vector<VkCommandBuffer> MT_CreateCommandBuffers(VkCommandPool commandPool, int num);
    void                         MT_ResetCommandPool(VkCommandPool commandPool);
    void                         MT_SubmitQueue(const std::vector<VkCommandBuffer>& commandBuffers, std::vector<VkSemaphore> waitSemaphore, VkSemaphore finishSemaphore, RIFence* fence);
#pragma region Command buffers

#pragma region Uniform buffers
    VulkanUniformBuffer* CreateUniformBuffer(uint32_t bytes);
    void                 DestroyUniformBuffer(VulkanUniformBuffer* ubo);
#pragma endregion

    void    MT_WaitIdle();
    VkEvent MT_CreateEvent();
    void    MT_DestroyEvent(VkEvent event);
    void    MT_ResetEvent(VkEvent event);

    VkRenderPass MT_CreateRenderPass(const DRenderPassAttachments& attachments);
    void         MT_DestroyRenderPass(VkRenderPass renderPass);

    VkQueryPool MT_CreateOcclusionQueryPool(uint32_t count);
    void        MT_QueryResults(VkQueryPool pool, uint32_t offset, uint32_t count, size_t bufferSize, void* pData);
    void        MT_DestroyQueryPool(VkQueryPool pool);

    inline static constexpr VkDescriptorType RIShaderBlockParam_TO_VkDescriptorType[4] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER };

    inline static constexpr VkFormat EImageFormat_TO_VkFormat[6] = { VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_B8G8R8A8_SRGB,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_R8_SRGB };

  protected:
    std::list<VkDescriptorPool>                                                                                                                                             _genericDescriptorPools;
    std::unique_ptr<RIDescriptorPoolManager>                                                                                                                                _globalDescriptorPool{};
    std::unordered_map<DRenderPassAttachments, VkRenderPass, DRenderPassAttachmentsHasher, DRenderPassAttachmentsEqualFn>                                                   _cachedRenderPass;
    RIVulkanBuffer                                                                                                                                                          _stagingBuffer{};
    unsigned char*                                                                                                                                                          _stagingBufferPtr{};
    std::unique_ptr<RingBufferManager>                                                                                                                                      _stagingBufferManager;
    RIVulkanBuffer                                                                                                                                                          _vertexIndexPool{};
    RIVulkanBuffer                                                                                                                                                          _uboSsboBufferPool{};
    std::unordered_map<std::vector<RIShaderDescriptorBindings>, std::unique_ptr<DRIDescriptorSetPool>, RIShaderDescriptorBindingsHasher, RIShaderDescriptorBindingsEqualFn> _cachedDescriptorSetPool;
    std::list<VulkanUniformBuffer>                                                                                                                                          _uniformBuffers;
    void _resetSwapchain(RISwapchainVk* swapchain) const;

    // NEW back end-------------------------------------------------------------------------------------------
    /*Per frame map of per pipeline layout descriptor pool*/
    std::unordered_map<VkPipelineLayout, std::map<uint32_t, VkDescriptorSetLayout>> _pipelineLayoutToSetIndexDescriptorSetLayout;
    std::vector<std::unordered_map<VkPipelineLayout, RIDescriptorPoolManager*>>     _pipelineLayoutToDescriptorPool;
    // NEW back end-------------------------------------------------------------------------------------------
  private:
    uint32_t _findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void     _transitionImageLayout(VkCommandBuffer cmd, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
};

struct RIShaderStageBuilderVk
{
    // The name of the main entry point in the shader
    inline static constexpr char* PENTRYPOINTNAME = "main";
    /*!
     * @brief Creates a VkPipelineShaderStageCreateInfo
     * @param stage the shader stage
     * @param shaderModule the shader module
     * @return VkPipelineShaderStageCreateInfo
     */
    inline static VkPipelineShaderStageCreateInfo Create(VkShaderStageFlagBits stage, VkShaderModule shaderModule);
};

struct RIShaderModuleBuilderVk
{
    inline static VkResult Create(VkDevice device, const std::vector<unsigned char>&, VkShaderModule* shaderModule);
};

struct RIVertexInputAttributeDescriptionBuilderVk
{
    inline static std::vector<VkVertexInputAttributeDescription> Create(const std::vector<RIInputSlot>& inputs);
};
}

#include "RIFactoryVk.inl"

#endif