// Copyright RedFox Studio 2022

#include "../App.h"

#include <fstream>
#include <iostream>
#include <string>

void
Log(const char* msg)
{
    std::cout << msg << std::endl;
};

inline std::vector<unsigned char>
ReadBlobUnsafe(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file.is_open())
        return {};

    std::ifstream::pos_type pos = file.tellg();

    // What happens if the OS supports really big files.
    // It may be larger than 32 bits?
    // This will silently truncate the value/
    std::streamoff length = pos;

    std::vector<unsigned char> bytes(length);
    if (pos > 0)
        { // Manuall memory management.
            // Not a good idea use a container/.
            file.seekg(0, std::ios::beg);
            file.read((char*)bytes.data(), length);
        }
    file.close();
    return bytes;
};

class TriangleApp : public App
{
  public:
    TriangleApp()
    {
        // Create vertex layout
        Fox::VertexLayoutInfo position("SV_POSITION", Fox::EFormat::R32G32B32_FLOAT, 0, Fox::EVertexInputClassification::PER_VERTEX_DATA);
        Fox::VertexLayoutInfo color("Color0", Fox::EFormat::R32G32B32A32_FLOAT, 3 * sizeof(float), Fox::EVertexInputClassification::PER_VERTEX_DATA);
        _vertexLayout             = _ctx->CreateVertexLayout({ position, color });
        constexpr uint32_t stride = 7 * sizeof(float);

        // Create shader
        Fox::ShaderSource shaderSource;
        shaderSource.VertexLayout            = _vertexLayout;
        shaderSource.VertexStride            = stride;
        shaderSource.SourceCode.VertexShader = ReadBlobUnsafe("vertex.spv");
        shaderSource.SourceCode.PixelShader  = ReadBlobUnsafe("fragment.spv");
        shaderSource.ColorAttachments        = 1;

        _shader = _ctx->CreateShader(shaderSource);
        // Create pipeline
        Fox::PipelineFormat          pipelineFormat;
        Fox::DFramebufferAttachments attachments;
        attachments.RenderTargets[0] = _swapchainRenderTargets[_swapchainImageIndex];

        _pipeline = _ctx->CreatePipeline(_shader, attachments, pipelineFormat);

        // clang-format off
        constexpr std::array<float, 21> ndcTriangle{            
            -1, -1, 0.5,//pos
            0, 1, 0, 1,// color
            1, -1, 0.5,//pos
            0, 0, 1, 1,// color
            0, 1, 0.5,//pos
            0, 1, 1, 1// color
        };
        // clang-format on
        constexpr size_t bufSize = sizeof(float) * ndcTriangle.size();
        _triangle                = _ctx->CreateBuffer(bufSize, Fox::VERTEX_INDEX_BUFFER, Fox::EMemoryUsage::RESOURCE_MEMORY_USAGE_CPU_ONLY);
        {
            // Copy vertices
            void* triangleData = _ctx->BeginMapBuffer(_triangle);
            memcpy(triangleData, (void*)ndcTriangle.data(), bufSize);
            _ctx->EndMapBuffer(_triangle);
        }
    };
    ~TriangleApp()
    {
        _ctx->WaitDeviceIdle();
        _ctx->DestroyShader(_shader);
        _ctx->DestroyPipeline(_pipeline);
        _ctx->DestroyBuffer(_triangle);
    };

    void Draw(uint32_t cmd, uint32_t w, uint32_t h)
    {
        _ctx->BeginCommandBuffer(cmd);

        Fox::DFramebufferAttachments attachments;
        attachments.RenderTargets[0] = _swapchainRenderTargets[_swapchainImageIndex];

        Fox::DLoadOpPass loadOp;
        loadOp.LoadColor[0]         = Fox::ERenderPassLoad::Clear;
        loadOp.ClearColor->color    = { 1, 1, 1, 1 };
        loadOp.StoreActionsColor[0] = Fox::ERenderPassStore::Store;
        _ctx->BindRenderTargets(cmd, attachments, loadOp);

        _ctx->BindPipeline(cmd, _pipeline);
        _ctx->SetViewport(cmd, 0, 0, w, h, 0.f, 1.f);
        _ctx->SetScissor(cmd, 0, 0, w, h);
        _ctx->BindVertexBuffer(cmd, _triangle);
        _ctx->Draw(cmd, 0, 3);

        Fox::RenderTargetBarrier presentBarrier;
        presentBarrier.RenderTarget  = _swapchainRenderTargets[_swapchainImageIndex];
        presentBarrier.mArrayLayer   = 1;
        presentBarrier.mCurrentState = Fox::EResourceState::RENDER_TARGET;
        presentBarrier.mNewState     = Fox::EResourceState::PRESENT;
        _ctx->ResourceBarrier(cmd, 0, nullptr, 0, nullptr, 1, &presentBarrier);

        _ctx->EndCommandBuffer(cmd);
    };

  protected:
    uint32_t _shader{};
    uint32_t _pipeline{};
    uint32_t _triangle{};
    uint32_t _vertexLayout{};
};

int
main()
{
    TriangleApp app;
    app.Run();

    return 0;
}