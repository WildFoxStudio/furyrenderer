// Copyright RedFox Studio 2022

#pragma once

#include <map>
#include <optional>
#include <vector>

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

struct DSwapchain_T
{
};

typedef DSwapchain_T* DSwapchain;

struct DFramebuffer_T
{
};

typedef DFramebuffer_T* DFramebuffer;

enum class EBufferType
{
    VERTEX_INDEX_BUFFER,
    UNIFORM_BUFFER_OBJECT,
    STORAGE_BUFFER_OBJECT,
};

struct DBuffer_T
{
    EBufferType Type;
};

typedef DBuffer_T* DBuffer;

struct DPipeline_T
{
};

typedef DPipeline_T* DPipeline;

struct DViewport
{
    unsigned int x, y, w, h, znear, zfar;
};

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

enum class EFormat
{
    R8_UNORM,
    R8G8B8_UNORM,
    R8G8B8A8_UNORM,
    B8G8R8_UNORM,
    B8G8R8A8_UNORM,
    DEPTH16_UNORM,
    DEPTH32_FLOAT,
    DEPTH16_UNORM_STENCIL8_UINT,
    DEPTH24_UNORM_STENCIL8_UINT,
    DEPTH32_FLOAT_STENCIL8_UINT,
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

struct DRenderPassAttachments
{
    std::vector<DRenderPassAttachment> Attachments;
};

struct SetBuffer
{
    DBuffer  Buffer{};
    uint32_t Offset{};
    uint32_t Range{};
};

// struct SetImage
//{
//     VkImageView   ImageView{};
//     VkImageLayout ImageLayout{};
//     VkSampler     Sampler{};
// };

enum class EBindingType
{
    UNIFORM_BUFFER_OBJECT,
    STORAGE_BUFFER_OBJECT,
    SAMPLER
};

struct SetBinding
{
    /* Only a buffer or image is a valid at once*/
    EBindingType         Type{};
    std::vector<DBuffer> Buffers;
    // std::vector<SetImage>  Images;
};

struct DrawCommand
{
    // Ctor
    DPipeline Pipeline;
    // Bind... functions
    std::map<uint32_t /*set*/, std::map<uint32_t /*binding*/, SetBinding>> DescriptorSetBindings;
    // Draw... functions
    DBuffer  VertexBuffer{};
    DBuffer  IndexBuffer{}; // optional
    uint32_t BeginVertex{};
    uint32_t VerticesCount{};

    DrawCommand(DPipeline pipeline) : Pipeline(pipeline) {}

    inline void BindBufferUniformBuffer(uint32_t set, uint32_t bindingIndex, DBuffer buffer)
    {
        SetBinding binding{ EBindingType::UNIFORM_BUFFER_OBJECT, { buffer } };
        DescriptorSetBindings[set][bindingIndex] = (std::move(binding));
    }

    /*inline void BindImageArray(uint32_t set, uint32_t bindingIndex, const std::vector<std::pair<VkImageView, VkSampler>>& imageToSamplerArray)
    {
        std::vector<SetImage> images(imageToSamplerArray.size());
        std::transform(imageToSamplerArray.begin(), imageToSamplerArray.end(), std::back_inserter(images), [](const std::pair<VkImageView, VkSampler>& p) {
            return SetImage{ p.first, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, p.second };
        });

        SetBinding binding{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, {}, images };
        DescriptorSetBindings[set][bindingIndex] = (std::move(binding));
    }*/

    inline void Draw(DBuffer vertexBuffer, uint32_t start, uint32_t count)
    {
        VertexBuffer  = vertexBuffer;
        BeginVertex   = start;
        VerticesCount = count;
    }
};

// struct CopyImageCommand
//{
//     VkImage                      Destination{};
//     std::vector<CopyMipMapLevel> MipMapCopy;
// };

struct CopyVertexCommand
{
    DBuffer                    Destination{};
    uint32_t                   DestOffset{};
    std::vector<unsigned char> Data;
};

struct CopyUniformBufferCommand
{
    DBuffer                    Destination{};
    uint32_t                   DestOffset{};
    std::vector<unsigned char> Data;
};

struct CopyDataCommand
{
    std::optional<CopyVertexCommand> VertexCommand;
    // std::optional<CopyImageCommand>         ImageCommand;
    std::optional<CopyUniformBufferCommand> UniformCommand;

    inline void CopyVertex(DBuffer destination, uint32_t destinationOffset, void* data, uint32_t bytes)
    {
        // check(!VertexCommand.has_value());
        //  check(!ImageCommand.has_value());
        // check(!UniformCommand.has_value());

        std::vector<unsigned char> blob(bytes, 0);
        memcpy(blob.data(), data, bytes);
        VertexCommand = CopyVertexCommand{ destination, destinationOffset, std::move(blob) };
    };

    inline void CopyUniformBuffer(DBuffer ubo, void* data, uint32_t bytes)
    {
        // check(!VertexCommand.has_value());
        //  check(!ImageCommand.has_value());
        // check(!UniformCommand.has_value());

        std::vector<unsigned char> blob(bytes, 0);
        memcpy(blob.data(), data, bytes);
        UniformCommand = CopyUniformBufferCommand{ ubo, 0, std::move(blob) };
    }

    // inline void CopyImageMipMap(VkImage destination, uint32_t offset, void* data, uint32_t width, uint32_t height, uint32_t mipMapIndex, uint32_t bytes)
    //{
    //     check(!VertexCommand.has_value());
    //     check(!ImageCommand.has_value());
    //     check(!UniformCommand.has_value());

    //    std::vector<unsigned char> blob(bytes, 0);
    //    memcpy(blob.data(), data, bytes);

    //    std::vector<CopyMipMapLevel> level;
    //    CopyMipMapLevel              l;
    //    l.Data     = std::move(blob);
    //    l.MipLevel = mipMapIndex;
    //    l.Width    = width;
    //    l.Height   = height;
    //    l.Offset   = offset;

    //    level.push_back(std::move(l));
    //    ImageCommand = CopyImageCommand{ destination, level };
    //}
};

struct RenderPassData
{
    DFramebuffer             Framebuffer{};
    DViewport                Viewport;
    DRenderPassAttachments   RenderPass;
    std::vector<DClearValue> ClearValues; // Equal to the RenderPass attachments with clear op
    std::vector<DrawCommand> DrawCommands;

    RenderPassData(DFramebuffer fbo, DViewport viewport, DRenderPassAttachments renderPass) : Framebuffer(fbo), Viewport(viewport), RenderPass(renderPass) {}
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

class IContext
{
  public:
    virtual ~IContext(){};
    virtual bool CreateSwapchain(const WindowData* windowData, EPresentMode& presentMode, EFormat& outFormat, DSwapchain* swapchain) = 0;
    virtual void DestroySwapchain(const DSwapchain swapchain)                                                                        = 0;

    virtual DFramebuffer CreateSwapchainFramebuffer()                 = 0;
    virtual void         DestroyFramebuffer(DFramebuffer framebuffer) = 0;

    virtual DBuffer CreateVertexBuffer(uint32_t size)   = 0;
    virtual void    DestroyVertexBuffer(DBuffer buffer) = 0;

    virtual void SubmitPass(RenderPassData&& data)  = 0;
    virtual void SubmitCopy(CopyDataCommand&& data) = 0;
    virtual void AdvanceFrame()                     = 0;
    virtual void FlushDeletedBuffers()              = 0;

    virtual unsigned char* GetAdapterDescription() const          = 0;
    virtual size_t         GetAdapterDedicatedVideoMemory() const = 0;
};
}