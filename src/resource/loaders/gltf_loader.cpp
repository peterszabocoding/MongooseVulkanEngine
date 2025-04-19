#include "gltf_loader.h"

#include <filesystem>
#include <glm/gtc/type_ptr.hpp>
#include <tiny_gltf/tiny_gltf.h>

#include "renderer/transform.h"
#include "renderer/mesh.h"
#include "renderer/vulkan/vulkan_material.h"
#include "renderer/vulkan/vulkan_mesh.h"
#include "resource/resource_manager.h"
#include "util/log.h"

namespace Raytracing
{
    namespace Utils
    {
        static void LoadGLTFNode(const tinygltf::Node& node, const tinygltf::Model& model, Ref<VulkanMesh>& vulkanMesh)
        {
            if (node.mesh <= -1) return;

            const tinygltf::Mesh mesh = model.meshes[node.mesh];
            for (size_t j = 0; j < mesh.primitives.size(); j++)
            {
                std::vector<Vertex> vertices;
                std::vector<uint32_t> indices;

                const tinygltf::Primitive& primitive = mesh.primitives[j];

                uint32_t vertexPos = 0;
                uint32_t indexPos = 0;
                uint32_t indexCount = 0;
                uint32_t vertexCount = 0;
                glm::vec3 posMin{};
                glm::vec3 posMax{};

                // Vertices
                {
                    const float* bufferPos = nullptr;
                    const float* bufferNormals = nullptr;
                    const float* bufferTexCoordSet0 = nullptr;
                    const float* bufferColorSet0 = nullptr;

                    int posByteStride = 0;
                    int normByteStride = 0;
                    int uv0ByteStride = 0;
                    int color0ByteStride = 0;

                    // Position attribute is required
                    assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

                    // POSITION
                    const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
                    const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];

                    bufferPos = reinterpret_cast<const float*>(&(model.buffers[posView.buffer].data[
                        posAccessor.byteOffset + posView.byteOffset]));

                    posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
                    posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);
                    vertexCount = static_cast<uint32_t>(posAccessor.count);
                    vertices.resize(vertexCount);
                    posByteStride = posAccessor.ByteStride(posView)
                                        ? (posAccessor.ByteStride(posView) / sizeof(float))
                                        : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);

                    // NORMAL
                    if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
                    {
                        const tinygltf::Accessor& normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
                        const tinygltf::BufferView& normView = model.bufferViews[normAccessor.bufferView];
                        bufferNormals = reinterpret_cast<const float*>(&(model.buffers[normView.buffer].data[
                            normAccessor.byteOffset + normView.byteOffset]));
                        normByteStride = normAccessor.ByteStride(normView)
                                             ? (normAccessor.ByteStride(normView) / sizeof(float))
                                             : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
                    }

                    // UVs
                    if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
                    {
                        const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
                        const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
                        bufferTexCoordSet0 = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[
                            uvAccessor.byteOffset + uvView.byteOffset]));
                        uv0ByteStride = uvAccessor.ByteStride(uvView)
                                            ? (uvAccessor.ByteStride(uvView) / sizeof(float))
                                            : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
                    }

                    // Vertex colors
                    if (primitive.attributes.find("COLOR_0") != primitive.attributes.end())
                    {
                        const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("COLOR_0")->second];
                        const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
                        bufferColorSet0 = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[
                            accessor.byteOffset + view.byteOffset]));
                        color0ByteStride = accessor.ByteStride(view)
                                               ? (accessor.ByteStride(view) / sizeof(float))
                                               : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
                    }


                    for (size_t v = 0; v < posAccessor.count; v++)
                    {
                        Vertex& vert = vertices[vertexPos];
                        vert.pos = glm::vec4(glm::make_vec3(&bufferPos[v * posByteStride]), 1.0f);
                        vert.texCoord = bufferTexCoordSet0 ? glm::make_vec2(&bufferTexCoordSet0[v * uv0ByteStride]) : glm::vec3(0.0f);
                        vert.color = bufferColorSet0 ? glm::make_vec4(&bufferColorSet0[v * color0ByteStride]) : glm::vec4(1.0f);
                        vert.normal = glm::normalize(
                            glm::vec3(bufferNormals
                                          ? glm::make_vec3(&bufferNormals[v * normByteStride])
                                          : glm::vec3(0.0f)));
                        vertexPos++;
                    }
                }

                // Indices
                bool hasIndices = primitive.indices > -1;
                if (hasIndices)
                {
                    const tinygltf::Accessor& accessor = model.accessors[primitive.indices > -1 ? primitive.indices : 0];
                    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

                    indexCount = static_cast<uint32_t>(accessor.count);
                    indices.resize(indexCount);
                    const void* dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

                    switch (accessor.componentType)
                    {
                        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                            const uint32_t* buf = static_cast<const uint32_t*>(dataPtr);
                            for (size_t index = 0; index < accessor.count; index++)
                            {
                                indices[indexPos] = buf[index];
                                indexPos++;
                            }
                            break;
                        }
                        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                            const uint16_t* buf = static_cast<const uint16_t*>(dataPtr);
                            for (size_t index = 0; index < accessor.count; index++)
                            {
                                indices[indexPos] = buf[index];
                                indexPos++;
                            }
                            break;
                        }
                        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                            const uint8_t* buf = static_cast<const uint8_t*>(dataPtr);
                            for (size_t index = 0; index < accessor.count; index++)
                            {
                                indices[indexPos] = buf[index];
                                indexPos++;
                            }
                            break;
                        }
                        default:
                            LOG_ERROR("Index component type ", accessor.componentType, " not supported!");
                            return;
                    }
                }

                vulkanMesh->AddMeshlet(vertices, indices, primitive.material);
            }
        }

        static void GetNodeProps(const tinygltf::Node& node, const tinygltf::Model& model, size_t& vertexCount, size_t& indexCount)
        {
            if (node.children.size() > 0)
            {
                for (size_t i = 0; i < node.children.size(); i++)
                    GetNodeProps(model.nodes[node.children[i]], model, vertexCount, indexCount);
            }

            if (node.mesh > -1)
            {
                const tinygltf::Mesh mesh = model.meshes[node.mesh];
                for (size_t i = 0; i < mesh.primitives.size(); i++)
                {
                    auto primitive = mesh.primitives[i];
                    vertexCount += model.accessors[primitive.attributes.find("POSITION")->second].count;
                    if (primitive.indices > -1)
                        indexCount += model.accessors[primitive.indices].count;
                }
            }
        }

        static std::vector<Ref<VulkanImage> > LoadTextures(tinygltf::Model& gltfModel, VulkanDevice* device,
                                                           std::filesystem::path parentPath)
        {
            std::vector<Ref<VulkanImage> > textures;
            for (tinygltf::Texture& tex: gltfModel.textures)
            {
                tinygltf::Image image = gltfModel.images[tex.source];
                std::string imagePath = parentPath.string() + "/" + image.uri;
                textures.push_back(ResourceManager::LoadTexture(device, imagePath));
            }
            return textures;
        }

        static std::vector<VulkanMaterial> LoadMaterials(tinygltf::Model& gltfModel, VulkanDevice* device,
                                                         const std::vector<Ref<VulkanImage> >& textures)
        {
            std::vector<VulkanMaterial> materials;
            for (const tinygltf::Material& material: gltfModel.materials)
            {
                VulkanMaterialBuilder builder(device);
                builder.SetPipeline(ResourceManager::GetMainPipeline());

                if (material.pbrMetallicRoughness.baseColorTexture.index >= 0)
                    builder.SetBaseColorTexture(textures[material.pbrMetallicRoughness.baseColorTexture.index]);

                if (material.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)
                    builder.SetMetallicRoughnessTexture(textures[material.pbrMetallicRoughness.metallicRoughnessTexture.index]);

                if (material.normalTexture.index >= 0)
                    builder.SetNormalMapTexture(textures[material.normalTexture.index]);

                materials.push_back(builder.Build());
            }
            return materials;
        }
    }

    Ref<VulkanMesh> GLTFLoader::LoadMesh(VulkanDevice* device, const std::string& meshPath)
    {
        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err;
        std::string warn;

        std::filesystem::path gltfFilePath(meshPath);
        const bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, meshPath);

        if (!warn.empty())
            LOG_TRACE("Warn: " + warn);
        if (!err.empty())
            LOG_TRACE("Err: " + err);

        if (!ret)
        {
            LOG_TRACE("Failed to parse glTF");
            abort();
        }

        const tinygltf::Scene& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];

        auto mesh = CreateRef<VulkanMesh>(device);
        for (size_t i = 0; i < scene.nodes.size(); i++)
        {
            size_t vertexCount = 0;
            size_t indexCount = 0;

            Utils::GetNodeProps(model.nodes[scene.nodes[i]], model, vertexCount, indexCount);

            std::vector<Vertex> vertices(vertexCount);
            std::vector<uint32_t> indices(indexCount);

            const tinygltf::Node node = model.nodes[scene.nodes[i]];
            Utils::LoadGLTFNode(node, model, mesh);
        }

        std::vector<Ref<VulkanImage> > textures = Utils::LoadTextures(model, device, gltfFilePath.parent_path());
        std::vector<VulkanMaterial> materials = Utils::LoadMaterials(model, device, textures);

        mesh->SetMaterials(materials);

        return mesh;
    }
}
