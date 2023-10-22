// Copyright RedFox Studio 2022

#include "../App.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <gli/gli.hpp>

#include <fstream>
#include <iostream>
#include <stdexcept>
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

struct ImageData
{
    uint32_t                   Width, Height;
    std::vector<unsigned char> Pixels;
};

inline std::vector<ImageData>
loadImage(const char* path, uint32_t* width, uint32_t* height, uint32_t* mipMaps, uint32_t* size)
{
    gli::texture Texture = gli::load(path);
    if (Texture.empty())
        throw std::runtime_error("Could not load image");

    gli::gl               GL(gli::gl::PROFILE_GL33);
    gli::gl::format const Format = GL.translate(Texture.format(), Texture.swizzles());
    assert(Format.Internal == gli::gl::INTERNAL_RGBA_DXT1);

    gli::extent2d extent = Texture.extent(0);
    *width               = extent.r;
    *height              = extent.g;
    *mipMaps             = Texture.levels();
    *size                = Texture.size();

    std::vector<ImageData> mips;
    for (uint32_t i = 0; i < *mipMaps; i++)
        {
            gli::extent2d ext     = Texture.extent(i);
            ImageData     mip     = { ext.r, ext.g };
            const auto    mipSize = Texture.size(i);
            mip.Pixels.resize(mipSize);
            memcpy(mip.Pixels.data(), Texture.data(0, 0, i), mipSize);

            mips.emplace_back(std::move(mip));
        }

    return mips;
}

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

        Fox::ShaderLayout shaderLayout;
        shaderLayout.SetsLayout[0].insert({ 0, Fox::ShaderDescriptorBindings{ "Camera", Fox::EBindingType::UNIFORM_BUFFER_OBJECT, sizeof(glm::mat4), 1, Fox::EShaderStage::VERTEX } });
        shaderLayout.SetsLayout[1].insert({ 0, Fox::ShaderDescriptorBindings{ "Texture", Fox::EBindingType::TEXTURE, NULL, 1, Fox::EShaderStage::FRAGMENT } });
        shaderLayout.SetsLayout[1].insert({ 1, Fox::ShaderDescriptorBindings{ "Sampler", Fox::EBindingType::SAMPLER, NULL, 1, Fox::EShaderStage::FRAGMENT } });

        _rootSignature = _ctx->CreateRootSignature(shaderLayout);
        _descriptorSet = _ctx->CreateDescriptorSets(_rootSignature, Fox::EDescriptorFrequency::NEVER, 1);
        _textureSet    = _ctx->CreateDescriptorSets(_rootSignature, Fox::EDescriptorFrequency::PER_FRAME, 1);

        _shader = _ctx->CreateShader(shaderSource);
        // Create pipeline
        Fox::PipelineFormat          pipelineFormat;
        Fox::DFramebufferAttachments attachments;
        attachments.RenderTargets[0] = _swapchainRenderTargets[_swapchainImageIndex];

        _pipeline = _ctx->CreatePipeline(_shader, _rootSignature, attachments, pipelineFormat);

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

        _cameraUbo = _ctx->CreateBuffer(sizeof(glm::mat4), Fox::EResourceType::UNIFORM_BUFFER, Fox::EMemoryUsage::RESOURCE_MEMORY_USAGE_CPU_ONLY);
        {
            void*     data = _ctx->BeginMapBuffer(_cameraUbo);
            glm::mat4 matrix(1.0f);
            memcpy(data, glm::value_ptr(matrix), sizeof(glm::mat4));
            _ctx->EndMapBuffer(_cameraUbo);
        }

        {
            uint32_t               w, h, m, s;
            std::vector<ImageData> mips = loadImage("texture.dds", &w, &h, &m, &s);

            _sampler = _ctx->CreateSampler(0, mips.size());

            _texture        = _ctx->CreateImage(Fox::EFormat::RGBA_DXT1, w, h, m);
            uint32_t tmpBuf = _ctx->CreateBuffer(s, Fox::EResourceType::TRANSFER, Fox::EMemoryUsage::RESOURCE_MEMORY_USAGE_CPU_ONLY);

            unsigned char* data = (unsigned char*)_ctx->BeginMapBuffer(tmpBuf);
            for (auto& mip : mips)
                {
                    memcpy(data, mip.Pixels.data(), mip.Pixels.size());
                    data += mip.Pixels.size();
                }
            _ctx->EndMapBuffer(tmpBuf);

            auto& cmd = _frameData[0].Cmd;

            _ctx->BeginCommandBuffer(cmd);
            Fox::TextureBarrier transferBarrier;
            transferBarrier.ImageId      = _texture;
            transferBarrier.CurrentState = Fox::EResourceState::UNDEFINED;
            transferBarrier.NewState     = Fox::EResourceState::COPY_DEST;

            _ctx->ResourceBarrier(cmd, 0, nullptr, 1, &transferBarrier, 0, nullptr);

            uint32_t offset{};
            for (auto mipIndex = 0; mipIndex < mips.size(); mipIndex++)
                {
                    auto& mip = mips[mipIndex];
                    _ctx->CopyImage(cmd, _texture, mip.Width, mip.Height, mipIndex, tmpBuf, offset);
                    offset += mip.Pixels.size();
                }

            Fox::TextureBarrier readBarrier;
            readBarrier.ImageId      = _texture;
            readBarrier.CurrentState = Fox::EResourceState::COPY_DEST;
            readBarrier.NewState     = Fox::EResourceState::SHADER_RESOURCE;

            _ctx->ResourceBarrier(cmd, 0, nullptr, 1, &readBarrier, 0, nullptr);

            _ctx->EndCommandBuffer(cmd);

            _ctx->QueueSubmit({}, {}, { cmd }, NULL);
            _ctx->WaitDeviceIdle();
            _ctx->DestroyBuffer(tmpBuf);

            _ctx->ResetCommandPool(_frameData[0].CmdPool);

            Fox::DescriptorData param[2] = {};
            param[0].pName               = "cameraUbo";
            param[0].Buffers             = &_cameraUbo;
            _ctx->UpdateDescriptorSet(_descriptorSet, 0, 1, param);

            param[0].pName    = "texture";
            param[0].Textures = &_texture;
            param[1].pName    = "sampler";
            param[1].Samplers = &_sampler;
            param[1].Index    = 1;
            _ctx->UpdateDescriptorSet(_textureSet, 0, 2, param);
        }
    };
    ~TriangleApp()
    {
        _ctx->WaitDeviceIdle();
        _ctx->DestroyShader(_shader);
        _ctx->DestroyPipeline(_pipeline);
        _ctx->DestroyBuffer(_triangle);
        _ctx->DestroyRootSignature(_rootSignature);
        _ctx->DestroyDescriptorSet(_descriptorSet);
        _ctx->DestroyDescriptorSet(_textureSet);
        _ctx->DestroyBuffer(_cameraUbo);
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

        _ctx->BindDescriptorSet(cmd, 0, _descriptorSet);
        _ctx->BindDescriptorSet(cmd, 0, _textureSet);
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
    uint32_t _rootSignature{};
    uint32_t _descriptorSet{};
    uint32_t _cameraUbo{};
    uint32_t _textureSet{};
    uint32_t _texture{};
    uint32_t _sampler{};
};

int
main()
{
    TriangleApp app;
    app.Run();

    return 0;
}