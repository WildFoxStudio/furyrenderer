// Copyright RedFox Studio 2022

#include "../App.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <gli/gli.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <tiny_gltf.h>
using namespace tinygltf;

// #include <stb_image.h>
#include <stb_image_resize2.h>

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#define GL_UNSIGNED_BYTE 0x1401
#define GL_SHORT 0x1402
#define GL_UNSIGNED_SHORT 0x1403
#define GL_INT 0x1404
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406

size_t
GlTypeToSize(const uint32_t glType)
{
    switch (glType)
        {
            case GL_UNSIGNED_BYTE:
                return 1;
                break;
            case GL_SHORT:
            case GL_UNSIGNED_SHORT:
                return 2;
                break;
            case GL_INT:
            case GL_UNSIGNED_INT:
            case GL_FLOAT:
                return 4;
                break;
            default:
                throw std::runtime_error("Invalid GL type");
                break;
        };
};

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
loadImage(const char* path, uint32_t* width, uint32_t* height, uint32_t* mipMaps, uint32_t* size, Fox::EFormat& format)
{
    gli::texture Texture = gli::load(path);
    if (Texture.empty())
        throw std::runtime_error("Could not load image");

    gli::gl         GL(gli::gl::PROFILE_GL33);
    gli::gl::format Format = GL.translate(Texture.format(), Texture.swizzles());
    // assert(Format.Internal == gli::gl::INTERNAL_RGBA_DXT1);

    switch (Format.Internal)
        {
            case gli::gl::INTERNAL_RGBA_DXT1:
                format = Fox::EFormat::RGBA_DXT1;
                break;

            case gli::gl::INTERNAL_RGBA_DXT3:
                format = Fox::EFormat::RGBA_DXT3;
                break;
            case gli::gl::INTERNAL_RGBA_DXT5:
                format = Fox::EFormat::RGBA_DXT5;
                break;

            default:
                assert("Invalid format file");
        }

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

inline std::vector<ImageData>
loadImageGenerateMipMaps(const char* path, uint32_t* width, uint32_t* height, uint32_t* mipMaps, uint32_t* size, Fox::EFormat& format)
{

    int            x, y, comps;
    unsigned char* image = stbi_load(path, &x, &y, &comps, 4);
    if (!image)
        throw std::runtime_error("Failed to load image");

    format  = Fox::EFormat::R8G8B8A8_UNORM;
    *width  = x;
    *height = y;
    *size   = (x * y) * 4;

    std::vector<ImageData> levels;
    {
        ImageData data{ x, y };
        data.Pixels.resize((x * y) * 4);
        memcpy(data.Pixels.data(), image, (x * y) * 4);
        levels.emplace_back(std::move(data));
    }

    stbi_image_free(image);

    // Gen mips

    while (x > 1 && y > 1)
        {
            x = x / 2;
            y = y / 2;
            *size += (x * y) * 4;

            ImageData data{ x, y };
            data.Pixels.resize((x * y) * 4);

            stbir_resize_uint8_linear(levels.back().Pixels.data(), levels.back().Width, levels.back().Height, 0, data.Pixels.data(), x, y, 0, STBIR_RGBA);

            levels.emplace_back(std::move(data));
        }
    *mipMaps = levels.size();
    return levels;
}

struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 Uv;
    int32_t   MaterialId{};
};

struct Submesh
{
    std::vector<Vertex>   Vertices;
    std::vector<uint32_t> Indices;
    std::string           Material;
};

struct MeshData
{
    std::string          Name{};
    std::vector<Submesh> Submeshes;
};

struct MeshMaterial
{
    std::string Name;
    std::string TextureName;
};

struct UboMaterial
{
    uint32_t AlbedoId{};
};

glm::mat4
computeViewMatrix(glm::vec3 rotation, glm::vec3 translation, glm::vec3& frontVector, glm::vec3& upVector)
{
    glm::quat qPitch = glm::angleAxis(glm::radians(rotation.x), glm::vec3(1, 0, 0));
    glm::quat qYaw   = glm::angleAxis(glm::radians(rotation.y), glm::vec3(0, 1, 0));
    glm::quat qRoll  = glm::angleAxis(glm::radians(rotation.z), glm::vec3(0, 0, 1));

    const glm::quat orientation    = glm::normalize(qPitch * qRoll * qYaw);
    const glm::mat4 rotationMatrix = glm::mat4_cast(orientation);

    // Update front and up vector
    frontVector = glm::normalize(glm::vec3(rotationMatrix[0][2], rotationMatrix[1][2], rotationMatrix[2][2]));
    upVector    = glm::normalize(glm::vec3(rotationMatrix[0][1], rotationMatrix[1][1], rotationMatrix[2][1]));

    const glm::mat4 viewMatrix = glm::translate(rotationMatrix, translation);

    return viewMatrix;
}

glm::mat4
computeProjectionMatrix(float width, float height, float fovRadians, float znear, float zfar)
{
    const float ratio = width / height;
    auto        mat   = glm::perspective(fovRadians, ratio, znear, zfar);
    // mat[1][1] *= -1; // y-coordinate is flipped for Vulkan
    return mat;
}

class TriangleApp : public App
{
  public:
    TriangleApp()
    {
        // Create vertex layout
        Fox::VertexLayoutInfo position("SV_POSITION", Fox::EFormat::R32G32B32_FLOAT, 0, Fox::EVertexInputClassification::PER_VERTEX_DATA);
        Fox::VertexLayoutInfo color("Color0", Fox::EFormat::R32G32B32A32_FLOAT, 4 * sizeof(float), Fox::EVertexInputClassification::PER_VERTEX_DATA);
        Fox::VertexLayoutInfo normal("NORMAL", Fox::EFormat::R32G32B32_FLOAT, 3 * sizeof(float), Fox::EVertexInputClassification::PER_VERTEX_DATA);
        Fox::VertexLayoutInfo texcoord("TEXCOORD", Fox::EFormat::R32G32_FLOAT, 6 * sizeof(float), Fox::EVertexInputClassification::PER_VERTEX_DATA);
        Fox::VertexLayoutInfo materialId("MATERIALID", Fox::EFormat::SINT32, 8 * sizeof(int32_t), Fox::EVertexInputClassification::PER_VERTEX_DATA);
        _vertexLayout             = _ctx->CreateVertexLayout({ position, normal, texcoord, materialId });
        constexpr uint32_t stride = 8 * sizeof(float) + sizeof(int32_t);

        // Create shader
        Fox::ShaderSource shaderSource;
        shaderSource.VertexLayout            = _vertexLayout;
        shaderSource.VertexStride            = stride;
        shaderSource.SourceCode.VertexShader = ReadBlobUnsafe("vertex.spv");
        shaderSource.SourceCode.PixelShader  = ReadBlobUnsafe("fragment.spv");
        shaderSource.ColorAttachments        = 1;

        Fox::ShaderLayout shaderLayout;
        shaderLayout.SetsLayout[0].insert({ 0, Fox::ShaderDescriptorBindings{ "Camera", Fox::EBindingType::UNIFORM_BUFFER_OBJECT, sizeof(glm::mat4), 1, Fox::EShaderStage::VERTEX } });
        shaderLayout.SetsLayout[1].insert({ 0, Fox::ShaderDescriptorBindings{ "Texture", Fox::EBindingType::TEXTURE, NULL, 1000, Fox::EShaderStage::FRAGMENT } });
        shaderLayout.SetsLayout[1].insert({ 1, Fox::ShaderDescriptorBindings{ "Sampler", Fox::EBindingType::SAMPLER, NULL, 1000, Fox::EShaderStage::FRAGMENT } });
        shaderLayout.SetsLayout[1].insert({ 2, Fox::ShaderDescriptorBindings{ "Material", Fox::EBindingType::UNIFORM_BUFFER_OBJECT, sizeof(UboMaterial), 1000, Fox::EShaderStage::FRAGMENT } });

        _rootSignature = _ctx->CreateRootSignature(shaderLayout);
        _descriptorSet = _ctx->CreateDescriptorSets(_rootSignature, Fox::EDescriptorFrequency::NEVER, 2);
        _textureSet    = _ctx->CreateDescriptorSets(_rootSignature, Fox::EDescriptorFrequency::PER_FRAME, 1);

        _shader = _ctx->CreateShader(shaderSource);

        // Create depth render target
        _depthRt = _ctx->CreateRenderTarget(Fox::EFormat::DEPTH32_FLOAT, Fox::ESampleBit::COUNT_1_BIT, true, WIDTH, HEIGHT, 1, 1, Fox::EResourceState::UNDEFINED);

        // Create pipeline
        Fox::PipelineFormat pipelineFormat;
        pipelineFormat.DepthTest     = true;
        pipelineFormat.DepthWrite    = true;
        pipelineFormat.DepthTestMode = Fox::EDepthTest::LESS;

        Fox::DPipelineAttachments attachments;
        attachments.RenderTargets[0] = format;
        attachments.DepthStencil     = Fox::EFormat::DEPTH32_FLOAT;

        _pipeline = _ctx->CreatePipeline(_shader, _rootSignature, attachments, pipelineFormat);

        // clang-format off
        constexpr float s = 100.f;
        constexpr std::array<float, 27> ndcTriangle{            
            -1*s, -1*s, 0.5*s,//pos
            0, 0, 1,// normal
            0,0,//uv
            0, // material Id
            1*s, -1*s, 0.5*s,//pos
            0, 0, 1,// normal
            1,0,//uv
            0, // material Id
            0, 1*s, 0.5*s,//pos
            0, 0, 1,// normal
            1,1,//uv
            0, // material Id
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

        _cameraLocation = glm::vec3(0.f);
        glm::vec3 up;
        _viewMatrix       = computeViewMatrix(_cameraRotation, _cameraLocation, _frontVector, up);
        _projectionMatrix = computeProjectionMatrix(WIDTH, HEIGHT, 70.f, 0.f, 1.f);

        _cameraUbo[0] = _ctx->CreateBuffer(sizeof(glm::mat4), Fox::EResourceType::UNIFORM_BUFFER, Fox::EMemoryUsage::RESOURCE_MEMORY_USAGE_CPU_ONLY);
        _cameraUbo[1] = _ctx->CreateBuffer(sizeof(glm::mat4), Fox::EResourceType::UNIFORM_BUFFER, Fox::EMemoryUsage::RESOURCE_MEMORY_USAGE_CPU_ONLY);

        {

            Model       model;
            TinyGLTF    loader;
            std::string err;
            std::string warn;

            bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, "Sponza/glTF/Sponza.gltf");

            if (!warn.empty())
                {
                    printf("Warn: %s\n", warn.c_str());
                }

            if (!err.empty())
                {
                    printf("Err: %s\n", err.c_str());
                }

            if (!ret)
                {
                    printf("Failed to parse glTF\n");
                }
            ret = loader.LoadBinaryFromFile(&model, &err, &warn, "Sponza/glTF/Sponza.gltf"); // for binary glTF(.glb)
            if (!warn.empty())
                {
                    printf("Warn: %s\n", warn.c_str());
                }

            if (!err.empty())
                {
                    printf("Err: %s\n", err.c_str());
                }

            if (!ret)
                {
                    printf("Failed to parse glTF\n");
                }

            // Load meshes
            for (const auto& scene : model.scenes)
                for (const auto& node : scene.nodes)
                    {
                        const auto& mesh = model.meshes[node];

                        MeshData meshData;
                        meshData.Name = mesh.name;

                        for (const auto& primitive : mesh.primitives)
                            {

                                Submesh submesh;
                                submesh.Material = model.materials[primitive.material].name;

                                // Index buffer
                                {
                                    const auto&  indexAccessor = model.accessors[primitive.indices];
                                    const auto&  indexBufView  = model.bufferViews[indexAccessor.bufferView];
                                    const size_t offset        = indexBufView.byteOffset + indexAccessor.byteOffset;
                                    size_t       valueBytes    = GlTypeToSize(indexAccessor.componentType);
                                    const size_t bufferViewEnd = offset + indexAccessor.count * valueBytes;
                                    const size_t validStride   = indexBufView.byteStride ? indexBufView.byteStride : valueBytes;
                                    submesh.Indices.reserve(indexAccessor.count);

                                    for (size_t i = offset; i < bufferViewEnd; i += validStride)
                                        {
                                            const unsigned char* bufferSlice = &model.buffers[indexBufView.buffer].data.at(i);

                                            unsigned char uByteData;
                                            memcpy(&uByteData, (void*)bufferSlice, sizeof(uByteData));
                                            unsigned short ushortData;
                                            memcpy(&ushortData, (void*)bufferSlice, sizeof(ushortData));
                                            uint32_t uintData;
                                            memcpy(&uintData, (void*)bufferSlice, sizeof(uintData));

                                            switch (indexAccessor.componentType)
                                                {
                                                    case GL_UNSIGNED_BYTE:
                                                        submesh.Indices.push_back(((uint32_t)uByteData));
                                                        break;
                                                    case GL_SHORT:
                                                    case GL_UNSIGNED_SHORT:

                                                        submesh.Indices.push_back(((uint32_t)ushortData));
                                                        break;
                                                    case GL_INT:
                                                    case GL_UNSIGNED_INT:
                                                    case GL_FLOAT:
                                                        submesh.Indices.push_back(((uint32_t)uintData));
                                                        break;
                                                    default:
                                                        throw std::runtime_error("Invalid GL type");
                                                        break;
                                                }
                                        }
                                }

                                std::vector<glm::vec3> positions;
                                std::vector<glm::vec2> uvs;
                                std::vector<glm::vec3> normals;
                                for (const auto& attribute : primitive.attributes)
                                    {
                                        const auto& accessor = model.accessors[attribute.second];
                                        const auto& bufView  = model.bufferViews[accessor.bufferView];
                                        const auto& buffer   = model.buffers[bufView.buffer];
                                        const auto  begin    = bufView.byteOffset + accessor.byteOffset;
                                        const auto  end      = begin + bufView.byteLength;

                                        if (attribute.first == "POSITION")
                                            {
                                                for (size_t i = begin; i < end; i += 3 * sizeof(float))
                                                    {
                                                        glm::vec3 pos;
                                                        memcpy(&pos[0], &buffer.data.at(i), sizeof(float) * 3);
                                                        positions.push_back(pos);
                                                    }
                                            }
                                        else if (attribute.first == "TEXCOORD_0")
                                            {
                                                for (size_t i = begin; i < end; i += 2 * sizeof(float))
                                                    {
                                                        glm::vec2 uv;
                                                        memcpy(&uv[0], &buffer.data.at(i), sizeof(float) * 2);
                                                        uvs.push_back(uv);
                                                    }
                                            }
                                        else if (attribute.first == "NORMAL")
                                            {
                                                for (size_t i = begin; i < end; i += 3 * sizeof(float))
                                                    {
                                                        glm::vec3 norm;
                                                        memcpy(&norm[0], &buffer.data.at(i), sizeof(float) * 3);
                                                        normals.push_back(norm);
                                                    }
                                            }
                                    }
                                assert(positions.size() == uvs.size());
                                assert(normals.size() == uvs.size());

                                for (size_t i = 0; i < positions.size(); i++)
                                    {
                                        Vertex v;
                                        v.Position = positions[i];
                                        v.Normal   = normals[i];
                                        v.Uv       = uvs[i];

                                        submesh.Vertices.push_back(v);
                                    }

                                meshData.Submeshes.push_back(submesh);
                            }

                        _meshes.push_back(meshData);
                    }

            for (const auto& mat : model.materials)
                {
                    MeshMaterial material;
                    material.Name               = mat.name;
                    const auto sourceImageIndex = model.textures.at(mat.pbrMetallicRoughness.baseColorTexture.index).source;
                    material.TextureName        = model.images[sourceImageIndex].uri;

                    _materials[mat.name] = material;
                }

            uint32_t cnt{ 1 };
            for (auto& image : model.images)
                {
                    std::string outName;
                    URIDecode(image.uri, &outName, nullptr);
                    // outName = outName.substr(0, outName.find_first_of('.', 0)) + ".dds";

                    if (_textures.find(outName) != _textures.end())
                        {
                            continue;
                        }

                    uint32_t tex, samp;

                    std::cout << "Loading texture:" << outName << " " << cnt++ << "/" << model.images.size() << std::endl;
                    _loadTexture("Sponza/glTF/" + outName, tex, samp);
                    _textures[outName] = std::make_pair(tex, samp);
                }
        }
        UboMaterial emptyMat;
        for (uint32_t i = 0; i < 1000; i++)
            {
                _materialUbos.push_back(_ctx->CreateBuffer(sizeof(UboMaterial), Fox::EResourceType::UNIFORM_BUFFER, Fox::EMemoryUsage::RESOURCE_MEMORY_USAGE_CPU_ONLY));
                void* mem = _ctx->BeginMapBuffer(_materialUbos.back());
                memcpy(mem, &emptyMat, sizeof(UboMaterial));
                _ctx->EndMapBuffer(_materialUbos.back());
            }

        // Build vertex and index buffer
        {
            _indirectVertex  = _ctx->CreateBuffer(sizeof(Vertex) * 1000000, Fox::EResourceType::VERTEX_INDEX_BUFFER, Fox::EMemoryUsage::RESOURCE_MEMORY_USAGE_CPU_ONLY);
            _indirectIndices = _ctx->CreateBuffer(sizeof(uint32_t) * 1000000, Fox::EResourceType::VERTEX_INDEX_BUFFER, Fox::EMemoryUsage::RESOURCE_MEMORY_USAGE_CPU_ONLY);

            uint32_t vertexOffset{};
            uint32_t indexOffset{};

            void* vertexMem = _ctx->BeginMapBuffer(_indirectVertex);
            void* indexMem  = _ctx->BeginMapBuffer(_indirectIndices);

            uint32_t uboMaterialCount{};
            for (const auto& mesh : _meshes)
                {
                    for (const auto& submesh : mesh.Submeshes)
                        {
                            // Append texture and sampler
                            const auto material           = _textures[_materials[submesh.Material].TextureName];
                            const auto textureShaderIndex = texArray.size();
                            texArray.push_back(material.first);
                            sampArray.push_back(material.second);

                            // Update ubo
                            {
                                UboMaterial uboData;
                                uboData.AlbedoId = textureShaderIndex;
                                uint32_t ubo     = _materialUbos[uboMaterialCount++];
                                void*    mem     = _ctx->BeginMapBuffer(ubo);
                                memcpy(mem, &uboData, sizeof(UboMaterial));
                                _ctx->EndMapBuffer(ubo);
                            }

                            // Update material id inside vertices
                            std::vector<Vertex> vertices = submesh.Vertices;
                            for (auto& vertex : vertices)
                                {
                                    vertex.MaterialId = uboMaterialCount - 1;
                                }

                            Fox::DrawIndexedIndirectCommand draw;
                            draw.firstIndex    = indexOffset / sizeof(uint32_t);
                            draw.indexCount    = submesh.Indices.size();
                            draw.instanceCount = 1;
                            draw.vertexOffset  = vertexOffset / sizeof(Vertex);
                            draw.firstInstance = 0;

                            _drawCommands.push_back(draw);

                            memcpy(((uint8_t*)vertexMem) + vertexOffset, vertices.data(), sizeof(Vertex) * vertices.size());
                            vertexOffset += sizeof(Vertex) * vertices.size();

                            memcpy(((uint8_t*)indexMem) + indexOffset, submesh.Indices.data(), sizeof(uint32_t) * submesh.Indices.size());
                            indexOffset += sizeof(uint32_t) * submesh.Indices.size();
                        }
                }

            _ctx->EndMapBuffer(_indirectVertex);
            _ctx->EndMapBuffer(_indirectIndices);
        }
        // Indirect buffer
        {
            _indirectBuffer = _ctx->CreateBuffer(sizeof(Fox::DrawIndexedIndirectCommand) * 1000, Fox::EResourceType::INDIRECT_DRAW_COMMAND, Fox::EMemoryUsage::RESOURCE_MEMORY_USAGE_CPU_ONLY);
            void* mem       = _ctx->BeginMapBuffer(_indirectBuffer);

            memcpy(mem, _drawCommands.data(), sizeof(Fox::DrawIndexedIndirectCommand) * _drawCommands.size());

            _ctx->EndMapBuffer(_indirectBuffer);
        }

        {
            _loadTexture("texture.jpg", _texture, _sampler);

            Fox::DescriptorData param[3] = {};
            param[0].pName               = "cameraUbo";
            param[0].Buffers             = &_cameraUbo[0];
            _ctx->UpdateDescriptorSet(_descriptorSet, 0, 1, param);
            param[0].Buffers = &_cameraUbo[1];
            _ctx->UpdateDescriptorSet(_descriptorSet, 1, 1, param);

            texArray.push_back(_texture);
            sampArray.push_back(_sampler);

            // for (const auto& texSamp : _textures)
            //     {
            //         texArray.push_back(texSamp.second.first);
            //         sampArray.push_back(texSamp.second.second);
            //     }

            param[0].pName    = "texture";
            param[0].Textures = texArray.data();
            param[0].Count    = texArray.size();
            param[1].pName    = "sampler";
            param[1].Samplers = sampArray.data();
            param[1].Index    = 1;
            param[1].Count    = sampArray.size();
            param[2].Index    = 2;
            param[2].Count    = _materialUbos.size();
            param[2].Buffers  = _materialUbos.data();
            _ctx->UpdateDescriptorSet(_textureSet, 0, 3, param);
        }
    };
    ~TriangleApp()
    {
        _ctx->WaitDeviceIdle();
        _ctx->DestroyShader(_shader);
        _ctx->DestroyRenderTarget(_depthRt);
        _ctx->DestroyPipeline(_pipeline);
        _ctx->DestroyBuffer(_triangle);
        _ctx->DestroyRootSignature(_rootSignature);
        _ctx->DestroyDescriptorSet(_descriptorSet);
        _ctx->DestroyDescriptorSet(_textureSet);
        _ctx->DestroyBuffer(_cameraUbo[0]);
        _ctx->DestroyBuffer(_cameraUbo[1]);
    };

    void RecreateSwapchain(uint32_t w, uint32_t h) override
    {
        _ctx->DestroyRenderTarget(_depthRt);
        _depthRt = _ctx->CreateRenderTarget(Fox::EFormat::DEPTH32_FLOAT, Fox::ESampleBit::COUNT_1_BIT, true, w, h, 1, 1, Fox::EResourceState::UNDEFINED);
    };

    void Draw(uint32_t cmd, uint32_t w, uint32_t h)
    {
        // Camera handling
        {
            float camSpeed = 0.1f;
            if (glfwGetKey(_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                {
                    glfwSetWindowShouldClose(_window, 1);
                }

            if (glfwGetKey(_window, GLFW_KEY_W) == GLFW_PRESS)
                {
                    _cameraLocation += _frontVector * camSpeed;
                }
            if (glfwGetKey(_window, GLFW_KEY_S) == GLFW_PRESS)
                {
                    _cameraLocation -= _frontVector * camSpeed;
                }
            if (glfwGetKey(_window, GLFW_KEY_D) == GLFW_PRESS)
                {
                    _cameraLocation += glm::cross(_frontVector, glm::vec3(0, 1, 0)) * camSpeed;
                }
            if (glfwGetKey(_window, GLFW_KEY_A) == GLFW_PRESS)
                {
                    _cameraLocation -= glm::cross(_frontVector, glm::vec3(0, 1, 0)) * camSpeed;
                }
            if (glfwGetMouseButton(_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
                {
                    _mouseMove = true;
                }
            if (glfwGetMouseButton(_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
                {
                    _mouseMove = false;
                }

            if (_mouseMove)
                {
                    double mx, my;
                    glfwGetCursorPos(_window, &mx, &my);
                    glm::ivec2 centerViewport(w / 2, h / 2);
                    glm::vec2  delta(centerViewport.x - mx, centerViewport.y - my);

                    if (glm::abs(delta.x) > 0.f || glm::abs(delta.y) > 0.f)
                        {
                            constexpr float cameraSensibility = 26.f;
                            _cameraRotation.x                 = glm::mod(_cameraRotation.x - delta.y / cameraSensibility, 360.0f);
                            _cameraRotation.y                 = glm::mod(_cameraRotation.y - delta.x / cameraSensibility, 360.0f);
                            glfwSetCursorPos(_window, w / 2, h / 2);
                        }
                }
            glm::vec3 up;
            _viewMatrix       = computeViewMatrix(_cameraRotation, _cameraLocation, _frontVector, up);
            _projectionMatrix = computeProjectionMatrix(w, h, 70.f, 0.1f, 100.f);

            {
                void*     data   = _ctx->BeginMapBuffer(_cameraUbo[_frameIndex]);
                glm::mat4 matrix = _projectionMatrix * _viewMatrix;
                memcpy(data, glm::value_ptr(matrix), sizeof(glm::mat4));
                _ctx->EndMapBuffer(_cameraUbo[_frameIndex]);
            }
        }

        _ctx->BeginCommandBuffer(cmd);

        Fox::DFramebufferAttachments attachments;
        attachments.RenderTargets[0] = _swapchainRenderTargets[_swapchainImageIndex];
        attachments.DepthStencil     = _depthRt;

        Fox::DLoadOpPass loadOp;
        loadOp.LoadColor[0]         = Fox::ERenderPassLoad::Clear;
        loadOp.ClearColor->color    = { 1, 1, 1, 1 };
        loadOp.StoreActionsColor[0] = Fox::ERenderPassStore::Store;
        loadOp.ClearDepthStencil    = { 1.f, 255 };
        loadOp.LoadDepth            = Fox::ERenderPassLoad::Clear;
        loadOp.StoreDepth           = Fox::ERenderPassStore::Store;
        _ctx->BindRenderTargets(cmd, attachments, loadOp);

        _ctx->BindPipeline(cmd, _pipeline);
        _ctx->SetViewport(cmd, 0, 0, w, h, 0.1f, 1.f);
        _ctx->SetScissor(cmd, 0, 0, w, h);
        _ctx->BindVertexBuffer(cmd, _triangle);

        _ctx->BindDescriptorSet(cmd, _frameIndex, _descriptorSet);
        _ctx->BindDescriptorSet(cmd, 0, _textureSet);
        // Draw triangle
        _ctx->Draw(cmd, 0, 3);

        _ctx->BindVertexBuffer(cmd, _indirectVertex);
        _ctx->BindIndexBuffer(cmd, _indirectIndices);

        _ctx->DrawIndexedIndirect(cmd, _indirectBuffer, 0, _drawCommands.size(), sizeof(Fox::DrawIndexedIndirectCommand));
        // for (const auto& draw : _drawCommands)
        //     {
        //         _ctx->DrawIndexed(cmd, draw.indexCount, draw.firstIndex, draw.vertexOffset);
        //     }

        Fox::RenderTargetBarrier presentBarrier;
        presentBarrier.RenderTarget  = _swapchainRenderTargets[_swapchainImageIndex];
        presentBarrier.mArrayLayer   = 1;
        presentBarrier.mCurrentState = Fox::EResourceState::RENDER_TARGET;
        presentBarrier.mNewState     = Fox::EResourceState::PRESENT;
        _ctx->ResourceBarrier(cmd, 0, nullptr, 0, nullptr, 1, &presentBarrier);

        _ctx->EndCommandBuffer(cmd);
    };

  protected:
    uint32_t                                                       _depthRt{};
    uint32_t                                                       _shader{};
    uint32_t                                                       _pipeline{};
    uint32_t                                                       _triangle{};
    uint32_t                                                       _vertexLayout{};
    uint32_t                                                       _rootSignature{};
    uint32_t                                                       _descriptorSet{};
    bool                                                           _mouseMove{};
    glm::vec3                                                      _cameraLocation{};
    glm::vec3                                                      _frontVector{};
    glm::vec3                                                      _cameraRotation{};
    glm::mat4                                                      _viewMatrix;
    glm::mat4                                                      _projectionMatrix;
    uint32_t                                                       _cameraUbo[MAX_FRAMES]{};
    uint32_t                                                       _textureSet{};
    uint32_t                                                       _texture{};
    uint32_t                                                       _sampler{};
    std::unordered_map<std::string, std::pair<uint32_t, uint32_t>> _textures;
    std::unordered_map<std::string, MeshMaterial>                  _materials;
    std::vector<MeshData>                                          _meshes;
    uint32_t                                                       _indirectVertex;
    uint32_t                                                       _indirectIndices;
    std::vector<Fox::DrawIndexedIndirectCommand>                   _drawCommands;
    std::vector<uint32_t>                                          _materialUbos;
    std::vector<uint32_t>                                          texArray;
    std::vector<uint32_t>                                          sampArray;
    uint32_t                                                       _indirectBuffer;

    bool _loadTexture(const std::string& filepath, uint32_t& texture, uint32_t& sampler)
    {
        uint32_t               w, h, m, s;
        Fox::EFormat           fmt;
        std::vector<ImageData> mips = loadImageGenerateMipMaps(filepath.c_str(), &w, &h, &m, &s, fmt);
        // std::vector<ImageData> mips = loadImage(filepath.c_str(), &w, &h, &m, &s, fmt);

        sampler = _ctx->CreateSampler(0, mips.size());

        texture         = _ctx->CreateImage(fmt, w, h, m);
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
        Fox::TextureBarrier transferBarrier{};
        transferBarrier.ImageId      = texture;
        transferBarrier.CurrentState = Fox::EResourceState::UNDEFINED;
        transferBarrier.NewState     = Fox::EResourceState::COPY_DEST;
        _ctx->ResourceBarrier(cmd, 0, nullptr, 1, &transferBarrier, 0, nullptr);

        uint32_t offset{};
        for (auto mipIndex = 0; mipIndex < mips.size(); mipIndex++)
            {
                auto& mip = mips[mipIndex];
                _ctx->CopyImage(cmd, texture, mip.Width, mip.Height, mipIndex, tmpBuf, offset);
                offset += mip.Pixels.size();
            }

        Fox::TextureBarrier readBarrier{};
        readBarrier.ImageId      = texture;
        readBarrier.CurrentState = Fox::EResourceState::COPY_DEST;
        readBarrier.NewState     = Fox::EResourceState::SHADER_RESOURCE;
        _ctx->ResourceBarrier(cmd, 0, nullptr, 1, &readBarrier, 0, nullptr);

        _ctx->EndCommandBuffer(cmd);

        _ctx->QueueSubmit({}, {}, { cmd }, NULL);
        _ctx->WaitDeviceIdle();
        _ctx->DestroyBuffer(tmpBuf);

        _ctx->ResetCommandPool(_frameData[0].CmdPool);

        return true;
    }
};

int
main()
{
    TriangleApp app;
    app.Run();

    return 0;
}