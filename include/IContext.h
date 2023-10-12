// Copyright RedFox Studio 2022

#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>
#include <array>

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
    VERTEX_INPUT_LAYOUT = 1,
    SHADER              = 2,
    VERTEX_INDEX_BUFFER = 3,
    UNIFORM_BUFFER      = 4,
    SWAPCHAIN           = 5,
    FRAMEBUFFER         = 6,
    IMAGE               = 7,
    GRAPHICS_PIPELINE   = 8,
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
    SAMPLER
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
    ShaderLayout        SetsLayout;
    VertexInputLayoutId VertexLayout{};
    uint32_t            VertexStride{};
    uint32_t            ColorAttachments{};
    bool                DepthStencilAttachment{};
};

struct SetBinding
{
    /* Only a buffer or image is a valid at once*/
    EBindingType          Type{};
    std::vector<BufferId> Buffers;
    std::vector<ImageId>  Images;
};

struct DrawCommand
{
    // Ctor
    ShaderId       Shader;
    PipelineFormat PipelineFormat;
    // Bind... functions
    std::map<uint32_t /*set*/, std::map<uint32_t /*binding*/, SetBinding>> DescriptorSetBindings;
    // Draw... functions
    BufferId VertexBuffer{};
    BufferId IndexBuffer{}; // optional
    uint32_t BeginVertex{};
    uint32_t VerticesCount{};

    DrawCommand(ShaderId shader) : Shader(shader) {}

    inline void BindBufferUniformBuffer(uint32_t set, uint32_t bindingIndex, BufferId buffer)
    {
        SetBinding binding{ EBindingType::UNIFORM_BUFFER_OBJECT, { buffer } };
        DescriptorSetBindings[set][bindingIndex] = (std::move(binding));
    }

    inline void BindImageArray(uint32_t set, uint32_t bindingIndex, const std::vector<ImageId>& imagesArray)
    {
        SetBinding binding{ EBindingType::SAMPLER, {}, imagesArray };
        DescriptorSetBindings[set][bindingIndex] = (std::move(binding));
    }

    inline void Draw(BufferId vertexBuffer, uint32_t start, uint32_t count)
    {
        VertexBuffer  = vertexBuffer;
        BeginVertex   = start;
        VerticesCount = count;
    }
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
    ImageId                      Destination{};
    std::vector<CopyMipMapLevel> MipMapCopy;
};

struct CopyVertexCommand
{
    BufferId                   Destination{};
    uint32_t                   DestOffset{};
    std::vector<unsigned char> Data;
};

struct CopyUniformBufferCommand
{
    BufferId                   Destination{};
    uint32_t                   DestOffset{};
    std::vector<unsigned char> Data;
};

struct CopyDataCommand
{
    std::optional<CopyVertexCommand>        VertexCommand;
    std::optional<CopyImageCommand>         ImageCommand;
    std::optional<CopyUniformBufferCommand> UniformCommand;

    inline void CopyVertex(BufferId destination, uint32_t destinationOffset, void* data, uint32_t bytes)
    {
        // check(!VertexCommand.has_value());
        //  check(!ImageCommand.has_value());
        // check(!UniformCommand.has_value());

        std::vector<unsigned char> blob(bytes, 0);
        memcpy(blob.data(), data, bytes);
        VertexCommand = CopyVertexCommand{ destination, destinationOffset, std::move(blob) };
    };

    inline void CopyUniformBuffer(BufferId ubo, void* data, uint32_t bytes)
    {
        // check(!VertexCommand.has_value());
        //  check(!ImageCommand.has_value());
        // check(!UniformCommand.has_value());

        std::vector<unsigned char> blob(bytes, 0);
        memcpy(blob.data(), data, bytes);
        UniformCommand = CopyUniformBufferCommand{ ubo, 0, std::move(blob) };
    }

    inline void CopyImageMipMap(ImageId destination, uint32_t offset, void* data, uint32_t width, uint32_t height, uint32_t mipMapIndex, uint32_t bytes)
    {
        // check(!VertexCommand.has_value());
        // check(!ImageCommand.has_value());
        // check(!UniformCommand.has_value());

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

struct RenderPassData
{
    FramebufferId            Framebuffer{};
    DViewport                Viewport;
    DRenderPassAttachments   RenderPass;
    std::vector<DClearValue> ClearValues; // Equal to the RenderPass attachments with clear op
    std::vector<DrawCommand> DrawCommands;

    RenderPassData(FramebufferId fbo, DViewport viewport, DRenderPassAttachments renderPass) : Framebuffer(fbo), Viewport(viewport), RenderPass(renderPass) {}
    inline void ClearColor(float r, float g, float b, float a = 1.f)
    {
        DClearColorValue col{ r, g, b, a };
        ClearValues.push_back({ col });
    }
    inline void ClearDepthStencil(float depth, uint32_t stencil)
    {
        DClearValue clearValue;
        clearValue.depthStencil = { depth, stencil };
        ClearValues.push_back(clearValue);
    }

    inline void AddDrawCommand(DrawCommand&& command) { DrawCommands.emplace_back(std::move(command)); }
};

struct DFramebufferAttachments
{
    static constexpr uint32_t             MAX_ATTACHMENTS = 10;
    std::array<uint32_t, MAX_ATTACHMENTS> ImageIds{};
};

struct DFramebufferAttachmentsHashFn
{
    size_t operator()(const DFramebufferAttachments& attachments)
    {
        size_t hash{};
        for (size_t i = 0; i < DFramebufferAttachments::MAX_ATTACHMENTS; i++)
            {
                hash += std::hash<uint32_t>{}(attachments.ImageIds[i]);
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
                if (lhs.ImageIds[i] != rhs.ImageIds[i])
                    {
                        return false;
                    }
            }
        return true;
    };
};

class IContext
{
  public:
    virtual ~IContext(){};
    virtual void                 WaitDeviceIdle()                                                                             = 0;
    virtual SwapchainId          CreateSwapchain(const WindowData* windowData, EPresentMode& presentMode, EFormat& outFormat) = 0;
    virtual std::vector<ImageId> GetSwapchainImages(SwapchainId swapchainId)                                                  = 0;
    virtual void                 DestroySwapchain(SwapchainId swapchainId)                                                    = 0;

    virtual FramebufferId CreateSwapchainFramebuffer_DEPRECATED(SwapchainId swapchainId) = 0;
    virtual void          DestroyFramebuffer(FramebufferId framebufferId)                = 0;

    virtual BufferId            CreateBuffer(uint32_t size, EResourceType type, EMemoryUsage usage)                                             = 0;
    virtual void*               BeginMapBuffer(BufferId buffer)                                                                                 = 0;
    virtual void                EndMapBuffer(BufferId buffer)                                                                                   = 0;
    virtual void                DestroyBuffer(BufferId buffer)                                                                                  = 0;
    virtual ImageId             CreateImage(EFormat format, uint32_t width, uint32_t height, uint32_t mipMapCount)                              = 0;
    virtual EFormat             GetImageFormat(ImageId) const                                                                                   = 0;
    virtual void                DestroyImage(ImageId imageId)                                                                                   = 0;
    virtual VertexInputLayoutId CreateVertexLayout(const std::vector<VertexLayoutInfo>& info)                                                   = 0;
    virtual ShaderId            CreateShader(const ShaderSource& source)                                                                        = 0;
    virtual void                DestroyShader(const ShaderId shader)                                                                            = 0;
    virtual uint32_t            CreatePipeline(const ShaderId shader, const DFramebufferAttachments& attachments, const PipelineFormat& format) = 0;
    virtual void                DestroyPipeline(uint32_t pipelineId)                                                                            = 0;

    virtual void SubmitPass(RenderPassData&& data)  = 0;
    virtual void SubmitCopy(CopyDataCommand&& data) = 0;
    virtual void AdvanceFrame()                     = 0;
    virtual void FlushDeletedBuffers()              = 0;

    virtual unsigned char* GetAdapterDescription() const          = 0;
    virtual size_t         GetAdapterDedicatedVideoMemory() const = 0;
};
}