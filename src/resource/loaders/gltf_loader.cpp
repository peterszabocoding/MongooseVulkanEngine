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
        struct VertexAttribute {
            const char* name;
            int type;
            const float* buffer = nullptr;
            int byteStride = -1;
        };

        static std::pair<const float*, int> ReadVertexValue(const tinygltf::Primitive& primitive, const tinygltf::Model& model,
                                                            const char* value, int type)
        {
            const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find(value)->second];
            const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];

            auto buffer = reinterpret_cast<const float*>(&model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]);

            int byteStride = accessor.ByteStride(view)
                                 ? accessor.ByteStride(view) / sizeof(float)
                                 : tinygltf::GetNumComponentsInType(type);

            return std::make_pair(buffer, byteStride);
        }

        static std::pair<glm::vec3, glm::vec3> ReadBoundingBoxValues(const tinygltf::Primitive& primitive, const tinygltf::Model& model)
        {
            const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("POSITION")->second];

            return std::make_pair(glm::vec3(accessor.minValues[0], accessor.minValues[1], accessor.minValues[2]),
                                  glm::vec3(accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2]));
        }

        static uint32_t GetVertexCount(const tinygltf::Primitive& primitive, const tinygltf::Model& model)
        {
            return model.accessors[primitive.attributes.find("POSITION")->second].count;
        }

        static bool HasAttribute(const tinygltf::Primitive& primitive, const char* attributeName)
        {
            return primitive.attributes.find(attributeName) != primitive.attributes.end();
        }

        static void LoadPrimitive(const tinygltf::Primitive& primitive, const tinygltf::Model& model, Ref<VulkanMesh> mesh)
        {
            std::array<VertexAttribute, 5> vAttributes{};
            vAttributes[0].name = "POSITION";
            vAttributes[0].type = TINYGLTF_TYPE_VEC3;

            vAttributes[1].name = "NORMAL";
            vAttributes[1].type = TINYGLTF_TYPE_VEC3;

            vAttributes[2].name = "TANGENT";
            vAttributes[2].type = TINYGLTF_TYPE_VEC4;

            vAttributes[3].name = "TEXCOORD_0";
            vAttributes[3].type = TINYGLTF_TYPE_VEC2;

            vAttributes[4].name = "COLOR_0";
            vAttributes[4].type = TINYGLTF_TYPE_VEC3;

            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;

            uint32_t vertexPos = 0;
            uint32_t indexPos = 0;
            uint32_t indexCount = 0;
            uint32_t vertexCount = 0;

            std::pair<glm::vec3, glm::vec3> boundingBox;

            // Vertices
            {
                // Position attribute is required
                assert(HasAttribute(primitive, "POSITION"));

                // POSITION
                vertexCount = GetVertexCount(primitive, model);
                vertices.resize(vertexCount);

                boundingBox = ReadBoundingBoxValues(primitive, model);

                for (auto& vertexAttribute: vAttributes)
                {
                    if (!HasAttribute(primitive, vertexAttribute.name)) continue;

                    auto [fst, snd] = ReadVertexValue(primitive, model, vertexAttribute.name, vertexAttribute.type);
                    vertexAttribute.buffer = fst;
                    vertexAttribute.byteStride = snd;
                }

                for (size_t v = 0; v < vertexCount; v++)
                {
                    Vertex& vert = vertices[vertexPos];

                    vert.pos = glm::vec4(glm::make_vec3(&vAttributes[0].buffer[v * vAttributes[0].byteStride]), 1.0f);

                    vert.texCoord = vAttributes[3].buffer
                                        ? glm::make_vec2(&vAttributes[3].buffer[v * vAttributes[3].byteStride])
                                        : glm::vec3(0.0f);

                    vert.color = vAttributes[4].buffer
                                     ? glm::make_vec4(&vAttributes[4].buffer[v * vAttributes[4].byteStride])
                                     : glm::vec4(1.0f);

                    vert.normal = normalize(glm::vec3(vAttributes[1].buffer
                                                          ? glm::make_vec3(&vAttributes[1].buffer[v * vAttributes[1].byteStride])
                                                          : glm::vec3(0.0f)));

                    glm::vec4 tempTangent = normalize(glm::vec4(vAttributes[2].buffer
                                                                    ? glm::make_vec4(&vAttributes[2].buffer[v * vAttributes[2].byteStride])
                                                                    : glm::vec4(0.0f)));

                    vert.tangent = normalize(glm::vec3(tempTangent));
                    vert.bitangent = normalize(cross(vert.normal, glm::vec3(tempTangent)) * tempTangent.w);

                    vertexPos++;
                }
            }

            // Indices
            if (primitive.indices > -1)
            {
                const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
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

            mesh->AddMeshlet(vertices, indices, primitive.material);
        }

        static void LoadGLTFNode(const tinygltf::Node& node, const tinygltf::Model& model, Ref<VulkanMesh>& vulkanMesh)
        {
            if (node.mesh <= -1) return;

            const tinygltf::Mesh mesh = model.meshes[node.mesh];
            for (auto& primitive: mesh.primitives)
            {
                LoadPrimitive(primitive, model, vulkanMesh);
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
            LOG_WARN("Warn: " + warn);
        if (!err.empty())
            LOG_ERROR("Err: " + err);

        if (!ret)
        {
            LOG_ERROR("Failed to parse glTF");
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

    Scene GLTFLoader::LoadScene(VulkanDevice* device, const std::string& scenePath)
    {
        return {};
    }
}
