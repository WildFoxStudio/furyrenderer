// Copyright RedFox Studio 2022

#pragma once

#include <array>
#include <map>
#include <optional>
#include <string>
#include <vector>

#define NOMINMAX

#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
#else
#pragma error Platform not supported
#endif

namespace Fox
{
// SHOULD BE PRIVATE
enum EResourceType : uint8_t
{
    TRANSFER              = 0,
    VERTEX_INPUT_LAYOUT   = 1,
    SHADER                = 2,
    VERTEX_INDEX_BUFFER   = 3,
    UNIFORM_BUFFER        = 4,
    SWAPCHAIN             = 5,
    FRAMEBUFFER           = 6,
    IMAGE                 = 7,
    GRAPHICS_PIPELINE     = 8,
    COMMAND_POOL          = 9,
    COMMAND_BUFFER        = 10,
    FENCE                 = 11,
    SEMAPHORE             = 12,
    RENDER_TARGET         = 13,
    ROOT_SIGNATURE        = 14,
    DESCRIPTOR_SET        = 15,
    SAMPLER               = 16,
    INDIRECT_DRAW_COMMAND = 17,
    QUEUE                 = 18,
};
// SHOULD BE PRIVATE

struct DContextConfig
{
    uint32_t stagingBufferSize{ 64 * 1024 * 1024 }; // 64mb
    void (*warningFunction)(const char*){};
    void (*logOutputFunction)(const char*){};
};

struct WindowData
{
#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)
    HINSTANCE _instance{};
    HWND      _hwnd{};
#elif defined(__linux__)
    Display* _display;
    Window   _window;
#else
#pragma error "Platform not supported"
#endif
};

enum class EPresentMode
{
    IMMEDIATE_KHR = 0,
    MAILBOX       = 1,
    FIFO          = 2,
    FIFO_RELAXED  = 3,
};

typedef uint32_t SwapchainId;
typedef uint32_t FramebufferId;
typedef uint32_t BufferId;
typedef uint32_t ShaderId;
typedef uint32_t VertexInputLayoutId;

enum class EBufferType
{
    VERTEX_INDEX_BUFFER,
    INDIRECT_DRAW_COMMAND,
    UNIFORM_BUFFER_OBJECT,
    STORAGE_BUFFER_OBJECT,
};

enum class EMemoryUsage
{
    /// Memory will be used on device only, no need to be mapped on host.
    RESOURCE_MEMORY_USAGE_GPU_ONLY = 1,
    /// Memory will be mapped on host. Could be used for transfer to device.
    RESOURCE_MEMORY_USAGE_CPU_ONLY = 2,
    /// Memory will be used for frequent (dynamic) updates from host and reads on device.
    RESOURCE_MEMORY_USAGE_CPU_TO_GPU = 3,
};

struct DPipeline_T
{
};

typedef DPipeline_T* DPipeline;

enum class EFormat
{
    INVALID = 0,
    R8_UNORM,
    R8G8B8_UNORM,
    R32_FLOAT,
    R32G32_FLOAT,
    R32G32B32_FLOAT,
    R32G32B32A32_FLOAT,
    R8G8B8A8_UNORM,
    B8G8R8_UNORM,
    B8G8R8A8_UNORM,
    DEPTH16_UNORM,
    DEPTH32_FLOAT,
    DEPTH16_UNORM_STENCIL8_UINT,
    DEPTH24_UNORM_STENCIL8_UINT,
    DEPTH32_FLOAT_STENCIL8_UINT,
    RGBA_DXT1,
    RGBA_DXT3,
    RGBA_DXT5,
    SINT32,
};

enum class EVertexInputClassification
{
    PER_VERTEX_DATA,
    PER_INSTANCE_DATA
};

struct VertexLayoutInfo
{

    const char*                Semantic;
    EFormat                    Format;
    uint32_t                   ByteOffset;
    EVertexInputClassification Classification;
    uint32_t                   InstanceDataStepRate;

    VertexLayoutInfo(const char* semantic, EFormat format, uint32_t byteOffset, EVertexInputClassification classification, uint32_t instanceDataStepRate = 0)
      : Semantic(semantic), Format(format), ByteOffset(byteOffset), Classification(classification), InstanceDataStepRate(instanceDataStepRate){};
};

enum class ETopology
{
    TRIANGLE_LIST,
    LINES_LIST
};

enum class EFillMode
{
    FILL,
    LINE
};
enum class ECullMode
{
    NONE,
    FRONT,
    BACK
};

enum class EDepthTest
{
    ALWAYS,
    NEVER,
    LESS,
    LESS_OR_EQUAL,
    GREATER,
    GREATER_OR_EQUAL
};

enum ERIBlendMode
{
    DefaultBlendMode,
    Additive
};

struct PipelineFormat
{
    ETopology    Topology{ ETopology::TRIANGLE_LIST };
    EFillMode    FillMode{ EFillMode::FILL };
    ECullMode    CullMode{ ECullMode::NONE };
    bool         DepthTest{ false };
    bool         DepthWrite{ false };
    EDepthTest   DepthTestMode{ EDepthTest::ALWAYS };
    bool         StencilTest{ false };
    ERIBlendMode BlendMode{ ERIBlendMode::DefaultBlendMode };
};

struct PipelineFormatHashFn
{
    size_t operator()(const PipelineFormat& lhs) const
    {
        size_t seed{};

        seed = std::hash<uint32_t>{}((uint32_t)lhs.Topology);
        seed += std::hash<uint32_t>{}((uint32_t)lhs.FillMode);
        seed += std::hash<uint32_t>{}((uint32_t)lhs.CullMode);
        seed ^= (uint32_t)lhs.DepthTest + (uint32_t)lhs.DepthWrite;
        seed ^= std::hash<uint32_t>{}((uint32_t)lhs.DepthTestMode + 1);
        seed += lhs.StencilTest;
        seed ^= std::hash<uint32_t>{}((uint32_t)lhs.BlendMode + 1);
        return seed;
    };
};

struct PipelineFormatEqualFn
{
    bool operator()(const PipelineFormat& lhs, const PipelineFormat& rhs) const
    {
        const bool topology      = lhs.Topology == rhs.Topology;
        const bool fillMode      = lhs.FillMode == rhs.FillMode;
        const bool cullMode      = lhs.CullMode == rhs.CullMode;
        const bool depthTest     = lhs.DepthTest == rhs.DepthTest;
        const bool depthWrite    = lhs.DepthWrite == rhs.DepthWrite;
        const bool depthTestMode = lhs.DepthTestMode == rhs.DepthTestMode;
        const bool stencilTest   = lhs.StencilTest == rhs.StencilTest;
        const bool blendMode     = lhs.BlendMode == rhs.BlendMode;

        return topology && fillMode && cullMode && depthTest && depthWrite && depthTestMode && stencilTest && blendMode;
    };
};

struct DViewport
{
    float x, y, w, h, znear, zfar;
};

typedef uint32_t ImageId;

typedef union DClearColorValue
{
    float    float32[4];
    int32_t  int32[4];
    uint32_t uint32[4];
} DClearColorValue;

typedef struct DClearDepthStencilValue
{
    float    depth;
    uint32_t stencil;
} DClearDepthStencilValue;

typedef union DClearValue
{
    DClearColorValue        color;
    DClearDepthStencilValue depthStencil;
} DClearValue;

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

enum class ESampleBit
{
    COUNT_1_BIT,
    COUNT_2_BIT,
    COUNT_4_BIT,
    COUNT_8_BIT,
    COUNT_16_BIT,
    COUNT_32_BIT,
    COUNT_64_BIT,
};

enum class EAttachmentReference
{
    COLOR_READ_ONLY,
    COLOR_ATTACHMENT,
    DEPTH_STENCIL_READ_ONLY,
    DEPTH_STENCIL_ATTACHMENT,
};

struct DRenderPassAttachment
{
    EFormat              Format;
    ESampleBit           Samples{ ESampleBit::COUNT_1_BIT };
    ERenderPassLoad      LoadOP;
    ERenderPassStore     StoreOP;
    ERenderPassLayout    InitialLayout;
    ERenderPassLayout    FinalLayout;
    EAttachmentReference AttachmentReferenceLayout;

    DRenderPassAttachment(EFormat format,
    ESampleBit                    sampleBit,
    ERenderPassLoad               loadOp,
    ERenderPassStore              storeOp,
    ERenderPassLayout             initLayout,
    ERenderPassLayout             finalLayout,
    EAttachmentReference          attachmentReferenceLayout)
      : Format(format), Samples(sampleBit), LoadOP(loadOp), StoreOP(storeOp), InitialLayout(initLayout), FinalLayout(finalLayout), AttachmentReferenceLayout(attachmentReferenceLayout){};
};

struct DRenderPassAttachmentHashFn
{
    size_t operator()(const DRenderPassAttachment& lhs) const
    {
        size_t seed{};

        seed ^= std::hash<size_t>{}((size_t)lhs.Format);
        seed += std::hash<size_t>{}((size_t)lhs.Samples);
        seed ^= std::hash<size_t>{}((size_t)lhs.LoadOP) + std::hash<size_t>{}((size_t)lhs.StoreOP);
        seed ^= std::hash<size_t>{}((size_t)lhs.InitialLayout) + std::hash<size_t>{}((size_t)lhs.FinalLayout);
        seed += std::hash<size_t>{}((size_t)lhs.AttachmentReferenceLayout);
        return seed;
    };
};

struct DRenderPassAttachmentsFormatsOnlyHashFn
{
    size_t operator()(const std::vector<DRenderPassAttachment>& lhs) const
    {
        size_t seed{ lhs.size() };
        for (const auto& format : lhs)
            {
                seed ^= std::hash<size_t>{}((size_t)format.Format * lhs.size());
            }
        return seed;
    };
};

struct DRenderPassAttachmentsEqualFn
{
    bool operator()(const DRenderPassAttachment& lhs, const DRenderPassAttachment& rhs) const
    {
        const bool format                    = lhs.Format == rhs.Format;
        const bool samples                   = lhs.Samples == rhs.Samples;
        const bool loadOP                    = lhs.LoadOP == rhs.LoadOP;
        const bool storeOP                   = lhs.StoreOP == rhs.StoreOP;
        const bool initialLayout             = lhs.InitialLayout == rhs.InitialLayout;
        const bool finalLayout               = lhs.FinalLayout == rhs.FinalLayout;
        const bool attachmentReferenceLayout = lhs.AttachmentReferenceLayout == rhs.AttachmentReferenceLayout;
        return format && samples && loadOP && storeOP && initialLayout && finalLayout && attachmentReferenceLayout;
    };
};

struct DRenderPassAttachmentsFormatsOnlyEqualFn
{
    bool operator()(const std::vector<DRenderPassAttachment>& lhs, const std::vector<DRenderPassAttachment>& rhs) const
    {
        if (lhs.size() != rhs.size())
            {
                return false;
            }

        for (size_t i = 0; i < lhs.size(); i++)
            {
                if (lhs[i].Format != rhs[i].Format)
                    {
                        return false;
                    }
            }

        return true;
    };
};

struct DRenderPassAttachments
{
    std::vector<DRenderPassAttachment> Attachments;
};

struct SetBuffer
{
    BufferId Buffer{};
    uint32_t Offset{};
    uint32_t Range{};
};

enum class EBindingType
{
    UNIFORM_BUFFER_OBJECT,
    STORAGE_BUFFER_OBJECT,
    TEXTURE,
    SAMPLER,
    COMBINED_IMAGE_SAMPLER,
};

enum class EShaderStage
{
    VERTEX,
    FRAGMENT,
    ALL
};

struct ShaderDescriptorBindings
{
    std::string  Name; // Optional
    EBindingType StorageType;
    size_t       Size; // Size in bytes of the block
    uint32_t     Count{ 1 }; // Used for array structures
    EShaderStage Stage;

    ShaderDescriptorBindings(std::string name, EBindingType type, size_t size, uint32_t elementCount, EShaderStage stage)
      : Name(name), StorageType(type), Size(size), Count(elementCount), Stage(stage){};

    inline size_t Hash() const
    {

        size_t                 hash = 0;
        std::hash<std::string> hasher;
        hash += hasher(Name);
        hash += (71 * hash + (size_t)StorageType) % 5;
        hash += (71 * hash + (size_t)Size) % 5;
        hash += (71 * hash + (size_t)Count) % 5;
        hash += (71 * hash + (size_t)Stage) % 5;
        return hash;
    };

    bool operator==(ShaderDescriptorBindings const& o) const
    {
        if (Name != o.Name)
            return false;

        if (StorageType != o.StorageType)
            return false;

        if (Size != o.Size)
            return false;

        if (Count != o.Count)
            return false;

        if (Stage != o.Stage)
            return false;

        return true;
    };
};

struct RIShaderDescriptorBindingsHasher
{
    size_t operator()(std::vector<ShaderDescriptorBindings> const& key) const
    {
        size_t hash = 0;
        for (size_t i = 0; i < key.size(); i++)
            {
                hash += key[i].Hash();
            }
        return hash;
    };
};
class RIShaderDescriptorBindingsEqualFn
{
  public:
    bool operator()(std::vector<ShaderDescriptorBindings> const& t1, std::vector<ShaderDescriptorBindings> const& t2) const
    {
        if (t1.size() != t2.size())
            return false;

        for (size_t i = 0; i < t1.size(); i++)
            {
                if (!(t1[i] == t2[i]))
                    return false;
            }

        return true;
    }
};

struct ShaderLayout
{
    std::map<uint32_t /*Set*/, std::map<uint32_t /*binding*/, ShaderDescriptorBindings>> SetsLayout;
};

struct ShaderByteCode
{
    // Compiled shader raw binary data
    std::vector<unsigned char> VertexShader;
    std::vector<unsigned char> PixelShader;
};

struct ShaderSource
{
    ShaderByteCode      SourceCode;
    VertexInputLayoutId VertexLayout{};
    uint32_t            VertexStride{};
    uint32_t            ColorAttachments{};
    bool                DepthStencilAttachment{};
};

struct DFramebufferAttachments
{
    //@TODO USE RAW ARRAY
    static constexpr uint32_t             MAX_ATTACHMENTS = 10;
    std::array<uint32_t, MAX_ATTACHMENTS> RenderTargets{};
    uint32_t                              DepthStencil{};
};

struct DPipelineAttachments
{
    //@TODO USE RAW ARRAY
    std::array<EFormat, DFramebufferAttachments::MAX_ATTACHMENTS> RenderTargets{};
    EFormat                                                       DepthStencil{};
};

struct DFramebufferAttachmentsHashFn
{
    size_t operator()(const DFramebufferAttachments& attachments)
    {
        size_t hash{};
        for (size_t i = 0; i < DFramebufferAttachments::MAX_ATTACHMENTS; i++)
            {
                hash += std::hash<uint32_t>{}(attachments.RenderTargets[i]);
            }
        return hash;
    };
};

struct DFramebufferAttachmentEqualFn
{
    bool operator()(const DFramebufferAttachments& lhs, const DFramebufferAttachments& rhs)
    {

        for (size_t i = 0; i < DFramebufferAttachments::MAX_ATTACHMENTS; i++)
            {
                if (lhs.RenderTargets[i] != rhs.RenderTargets[i])
                    {
                        return false;
                    }
            }
        return true;
    };
};

struct DLoadOpPass
{
    ERenderPassLoad  LoadColor[DFramebufferAttachments::MAX_ATTACHMENTS];
    ERenderPassLoad  LoadDepth;
    ERenderPassLoad  LoadStencil;
    DClearValue      ClearColor[DFramebufferAttachments::MAX_ATTACHMENTS];
    DClearValue      ClearDepthStencil;
    ERenderPassStore StoreActionsColor[DFramebufferAttachments::MAX_ATTACHMENTS];
    ERenderPassStore StoreDepth;
    ERenderPassStore StoreStencil;
};

enum class EResourceState
{
    UNDEFINED                         = 0,
    VERTEX_AND_CONSTANT_BUFFER        = 0x1,
    INDEX_BUFFER                      = 0x2,
    RENDER_TARGET                     = 0x4,
    UNORDERED_ACCESS                  = 0x8,
    DEPTH_WRITE                       = 0x10,
    DEPTH_READ                        = 0x20,
    NON_PIXEL_SHADER_RESOURCE         = 0x40,
    PIXEL_SHADER_RESOURCE             = 0x80,
    SHADER_RESOURCE                   = 0x40 | 0x80,
    STREAM_OUT                        = 0x100,
    INDIRECT_ARGUMENT                 = 0x200,
    COPY_DEST                         = 0x400,
    COPY_SOURCE                       = 0x800,
    GENERAL_READ                      = (((((0x1 | 0x2) | 0x40) | 0x80) | 0x200) | 0x800),
    PRESENT                           = 0x1000,
    COMMON                            = 0x2000,
    RAYTRACING_ACCELERATION_STRUCTURE = 0x4000,
    SHADING_RATE_SOURCE               = 0x8000,
};

enum EQueueType
{
    QUEUE_GRAPHICS = 0x1,
    QUEUE_TRANSFER = 0x2,
    QUEUE_COMPUTE  = 0x4,
};

enum class EPipelineType
{
    GRAPHICS = 0,
    COMPUTE,
    RAYTRACING
};

enum class EDescriptorFrequency : uint32_t
{
    NEVER = 0,
    PER_FRAME,
    PER_BATCH,
    PER_DRAW,
    MAX_COUNT
};

enum class EDescriptorType : int
{
    STATIC = 0,
    DYNAMIC,
    STORAGE,
    SAMPLER_COMBINED
};

enum class ERIShaderStage
{
    VERTEX,
    FRAGMENT,
    ALL
};

struct DescriptorSetDesc
{
    std::string     Name; // Optional
    uint32_t        Binding; // Binding index in shader
    EDescriptorType Type;
    uint32_t        Count = 1; // Used for array structures
    size_t          Size; // Size in bytes of the block
    ERIShaderStage  Stage;
};

enum ETransferOwnership : uint8_t
{
    NONE = 0,
    ACQUIRE,
    RELEASE
};

typedef struct BufferBarrier
{
    uint32_t           BufferId;
    EResourceState     CurrentState;
    EResourceState     NewState;
    uint8_t            BeginOnly : 1;
    uint8_t            EndOnly : 1;
    ETransferOwnership TransferOwnership{ ETransferOwnership::NONE };
    uint32_t           SrcQueue{};
    uint32_t           DstQueue{};
} BufferBarrier;

typedef struct TextureBarrier
{
    uint32_t           ImageId;
    EResourceState     CurrentState;
    EResourceState     NewState;
    uint8_t            BeginOnly : 1;
    uint8_t            EndOnly : 1;
    ETransferOwnership TransferOwnership{ ETransferOwnership::NONE };
    uint32_t           SrcQueue{};
    uint32_t           DstQueue{};
    /// Specifiy whether following barrier targets particular subresource
    uint8_t mSubresourceBarrier : 1;
    /// Following values are ignored if mSubresourceBarrier is false
    uint8_t  mMipLevel : 7;
    uint16_t mArrayLayer;
} TextureBarrier;

typedef struct RenderTargetBarrier
{
    uint32_t           RenderTarget{};
    EResourceState     mCurrentState;
    EResourceState     mNewState;
    uint8_t            mBeginOnly : 1;
    uint8_t            mEndOnly : 1;
    ETransferOwnership TransferOwnership{ ETransferOwnership::NONE };
    uint32_t           SrcQueue{};
    uint32_t           DstQueue{};
    /// Specifiy whether following barrier targets particular subresource
    uint8_t mSubresourceBarrier : 1;
    /// Following values are ignored if mSubresourceBarrier is false
    uint8_t  mMipLevel : 7;
    uint16_t mArrayLayer{};
} RenderTargetBarrier;

typedef struct DescriptorData
{
    const char* pName;
    uint32_t    Count{};
    uint32_t    ArrayOffset{};
    uint32_t    Index{};

    union
    {
        uint32_t* Textures;
        uint32_t* Samplers;
        uint32_t* Buffers;
    };
} DescriptorData;

typedef struct DrawIndexedIndirectCommand
{
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int32_t  vertexOffset;
    uint32_t firstInstance;
} DrawIndexedIndirectCommand;

class IContext
{
  public:
    virtual ~IContext(){};
    virtual void                  WaitDeviceIdle()                                                                                                                                    = 0;
    virtual uint32_t              CreateSwapchain(const WindowData* windowData, EPresentMode& presentMode, EFormat& outFormat, uint32_t* width = nullptr, uint32_t* height = nullptr) = 0;
    virtual std::vector<uint32_t> GetSwapchainRenderTargets(uint32_t swapchainId)                                                                                                     = 0;
    virtual bool                  SwapchainAcquireNextImageIndex(uint32_t swapchainId, uint64_t timeoutNanoseconds, uint32_t sempahoreid, uint32_t* outImageIndex)                    = 0;
    virtual void                  DestroySwapchain(uint32_t swapchainId)                                                                                                              = 0;

    /**
     * \brief If exists will return a specialized queue, otherwise if exists will return a queue with that feature, otherwise it will return a general purpose queue that supports that feature. If no
     * queue with requested feature are available it will return 0. It will return as many queues as there are available, after it will return always a general purpose queue with same id. Don't loose
     * the queue id because specialized queues return only once.
     * \param queueTypeFlag
     * \return A queue with the requested feature
     */
    virtual uint32_t FindQueue(EQueueType queueType) = 0;

    virtual uint32_t CreateBuffer(uint32_t size, EResourceType type, EMemoryUsage usage)                                                                    = 0;
    virtual void*    BeginMapBuffer(uint32_t buffer)                                                                                                        = 0;
    virtual void     EndMapBuffer(uint32_t buffer)                                                                                                          = 0;
    virtual void     DestroyBuffer(uint32_t buffer)                                                                                                         = 0;
    virtual ImageId  CreateImage(EFormat format, uint32_t width, uint32_t height, uint32_t mipMapCount)                                                     = 0;
    virtual EFormat  GetImageFormat(ImageId) const                                                                                                          = 0;
    virtual void     DestroyImage(ImageId imageId)                                                                                                          = 0;
    virtual uint32_t CreateVertexLayout(const std::vector<VertexLayoutInfo>& info)                                                                          = 0;
    virtual uint32_t CreateShader(const ShaderSource& source)                                                                                               = 0;
    virtual void     DestroyShader(const uint32_t shader)                                                                                                   = 0;
    virtual uint32_t CreatePipeline(const uint32_t shader, uint32_t rootSignatureId, const DPipelineAttachments& attachments, const PipelineFormat& format) = 0;
    virtual void     DestroyPipeline(uint32_t pipelineId)                                                                                                   = 0;
    virtual uint32_t CreateRootSignature(const ShaderLayout& layout)                                                                                        = 0;
    virtual void     DestroyRootSignature(uint32_t rootSignatureId)                                                                                         = 0;
    virtual uint32_t CreateDescriptorSets(uint32_t rootSignatureId, EDescriptorFrequency frequency, uint32_t count)                                         = 0;
    virtual void     DestroyDescriptorSet(uint32_t descriptorSetId)                                                                                         = 0;
    virtual void     UpdateDescriptorSet(uint32_t descriptorSetId, uint32_t setIndex, uint32_t paramCount, DescriptorData* params)                          = 0;
    virtual uint32_t CreateSampler(uint32_t minLod, uint32_t maxLod)                                                                                        = 0;

    virtual uint32_t CreateCommandPool(uint32_t queueId)                                                                                                                            = 0;
    virtual void     DestroyCommandPool(uint32_t commandPoolId)                                                                                                                     = 0;
    virtual void     ResetCommandPool(uint32_t commandPoolId)                                                                                                                       = 0;
    virtual uint32_t CreateCommandBuffer(uint32_t commandPoolId)                                                                                                                    = 0;
    virtual void     DestroyCommandBuffer(uint32_t commandBufferId)                                                                                                                 = 0;
    virtual void     BeginCommandBuffer(uint32_t commandBufferId)                                                                                                                   = 0;
    virtual void     EndCommandBuffer(uint32_t commandBufferId)                                                                                                                     = 0;
    virtual void     BindRenderTargets(uint32_t commandBufferId, const DFramebufferAttachments& attachments, const DLoadOpPass& loadOP)                                             = 0;
    virtual void     SetViewport(uint32_t commandBufferId, uint32_t x, uint32_t y, uint32_t width, uint32_t height, float znear, float zfar)                                        = 0;
    virtual void     SetScissor(uint32_t commandBufferId, uint32_t x, uint32_t y, uint32_t width, uint32_t height)                                                                  = 0;
    virtual void     BindPipeline(uint32_t commandBufferId, uint32_t pipeline)                                                                                                      = 0;
    virtual void     BindVertexBuffer(uint32_t commandBufferId, uint32_t bufferId)                                                                                                  = 0;
    virtual void     BindIndexBuffer(uint32_t commandBufferId, uint32_t bufferId)                                                                                                   = 0;
    virtual void     Draw(uint32_t commandBufferId, uint32_t firstVertex, uint32_t count)                                                                                           = 0;
    virtual void     DrawIndexed(uint32_t commandBufferId, uint32_t index_count, uint32_t first_index, uint32_t first_vertex)                                                       = 0;
    virtual void     DrawIndexedIndirect(uint32_t commandBufferId, uint32_t buffer, uint32_t offset, uint32_t drawCount, uint32_t stride)                                           = 0;
    virtual void     BindDescriptorSet(uint32_t commandBufferId, uint32_t setIndex, uint32_t descriptorSetId)                                                                       = 0;
    virtual void     CopyImage(uint32_t commandId, uint32_t imageId, uint32_t width, uint32_t height, uint32_t mipMapIndex, uint32_t stagingBufferId, uint32_t stagingBufferOffset) = 0;

    virtual uint32_t CreateRenderTarget(EFormat format, ESampleBit samples, bool isDepth, uint32_t width, uint32_t height, uint32_t arrayLength, uint32_t mipMapCount, EResourceState initialState) = 0;
    virtual void     DestroyRenderTarget(uint32_t renderTargetId)                                                                                                                                   = 0;
    virtual void     ResourceBarrier(uint32_t commandBufferId,
        uint32_t                              buffer_barrier_count,
        BufferBarrier*                        p_buffer_barriers,
        uint32_t                              texture_barrier_count,
        TextureBarrier*                       p_texture_barriers,
        uint32_t                              rt_barrier_count,
        RenderTargetBarrier*                  p_rt_barriers)                                                                                                                                                         = 0;

    virtual uint32_t CreateFence(bool signaled)                                  = 0;
    virtual void     DestroyFence(uint32_t fenceId)                              = 0;
    virtual bool     IsFenceSignaled(uint32_t fenceId)                           = 0;
    virtual void     WaitForFence(uint32_t fenceId, uint64_t timeoutNanoseconds) = 0;
    virtual void     ResetFence(uint32_t fenceId)                                = 0;

    /**
     * \brief Must be externally synchronized if called from multiple threads
     * \param waitSemaphore
     * \param finishSemaphore
     * \param cmdIds
     * \param fenceId
     */
    virtual void QueueSubmit(uint32_t queueId, const std::vector<uint32_t>& waitSemaphore, const std::vector<uint32_t>& finishSemaphore, const std::vector<uint32_t>& cmdIds, uint32_t fenceId) = 0;
    virtual void QueuePresent(uint32_t queueId, uint32_t swapchainId, uint32_t imageIndex, const std::vector<uint32_t>& waitSemaphore)                                                          = 0;

    virtual uint32_t CreateGpuSemaphore()                      = 0;
    virtual void     DestroyGpuSemaphore(uint32_t semaphoreId) = 0;

    virtual void FlushDeletedBuffers() = 0;

    virtual unsigned char* GetAdapterDescription() const          = 0;
    virtual size_t         GetAdapterDedicatedVideoMemory() const = 0;
};
}