#include "resource/loaders/gltf_loader.h"

#include <filesystem>
#include <glm/gtc/type_ptr.hpp>

#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf/tiny_gltf.h>

#include "renderer/vulkan/vulkan_device.h"
#include "renderer/transform.h"
#include "renderer/mesh.h"
#include "renderer/vulkan/vulkan_material.h"
#include "renderer/vulkan/vulkan_mesh.h"
#include "renderer/vulkan/vulkan_renderer.h"
#include "resource/resource_manager.h"
#include "util/log.h"
#include "util/timer.h"

namespace MongooseVK
{
    namespace Utils
    {
        struct VertexAttribute {
            const char* name;
            int type;
            const float* buffer = nullptr;
            int byteStride = -1;
        };

        struct glTFPrimitive {
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
            int materialIndex = -1;
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

        static glTFPrimitive LoadPrimitive(const tinygltf::Primitive& primitive, const tinygltf::Model& model)
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

            // Vertices
            {
                // Position attribute is required
                assert(HasAttribute(primitive, "POSITION"));

                // POSITION
                uint32_t vertexCount = GetVertexCount(primitive, model);
                vertices.resize(vertexCount);
                uint32_t vertexPos = 0;

                std::pair<glm::vec3, glm::vec3> boundingBox = ReadBoundingBoxValues(primitive, model);

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
                                        : glm::vec2(0.0f);

                    vert.color = vAttributes[4].buffer
                                     ? glm::make_vec3(&vAttributes[4].buffer[v * vAttributes[4].byteStride])
                                     : glm::vec3(1.0f);

                    vert.normal = normalize(glm::vec3(vAttributes[1].buffer
                                                          ? glm::make_vec3(&vAttributes[1].buffer[v * vAttributes[1].byteStride])
                                                          : glm::vec3(0.0f)));

                    vert.tangent = normalize(glm::vec3(vAttributes[2].buffer
                                                           ? glm::make_vec3(&vAttributes[2].buffer[v * vAttributes[2].byteStride])
                                                           : glm::vec3(0.0f)));

                    vert.bitangent = normalize(cross(vert.normal, vert.tangent));

                    vertexPos++;
                }
            }

            // Indices
            if (primitive.indices > -1)
            {
                uint32_t indexCount = 0;
                uint32_t indexPos = 0;
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
                        ASSERT(false, "Error loading mesh primitive");
                        return {};
                }
            }

            glTFPrimitive result{};
            result.vertices = vertices;
            result.indices = indices;
            result.materialIndex = primitive.material;

            return result;
        }

        static void LoadGLTFNode(const tinygltf::Node& node,
                                 const tinygltf::Model& model,
                                 const Ref<VulkanMesh>& vulkanMesh,
                                 const std::vector<MaterialHandle>& materials)
        {
            if (node.mesh <= -1) return;

            for (auto& primitive: model.meshes[node.mesh].primitives)
            {
                glTFPrimitive meshPrimitive = LoadPrimitive(primitive, model);
                vulkanMesh->AddMeshlet(meshPrimitive.vertices, meshPrimitive.indices, materials[meshPrimitive.materialIndex]);
            }
        }
    }


    std::vector<TextureHandle> GLTFLoader::LoadTextures(VulkanDevice* device, const tinygltf::Model& model,
                                                        const std::filesystem::path& parentPath)
    {
        std::vector<TextureHandle> textures;
        for (const tinygltf::Texture& tex: model.textures)
        {
            tinygltf::Image image = model.images[tex.source];
            std::string imagePath = parentPath.string() + "/" + image.uri;

            ResourceManager::LoadTexture( device, imagePath, &textures);
        }

        return textures;
    }

    std::vector<MaterialHandle> GLTFLoader::LoadMaterials(VulkanDevice* device, const tinygltf::Model& model,
                                                          const std::filesystem::path& parentPath)
    {
        const std::vector<TextureHandle>& textureHandles = LoadTextures(device, model, parentPath);

        std::vector<MaterialHandle> materials;
        for (const tinygltf::Material& material: model.materials)
        {
            MaterialCreateInfo materialCreateInfo{};
            materialCreateInfo.baseColor = glm::make_vec4(material.pbrMetallicRoughness.baseColorFactor.data());
            materialCreateInfo.metallic = material.pbrMetallicRoughness.metallicFactor;
            materialCreateInfo.roughness = material.pbrMetallicRoughness.roughnessFactor;
            materialCreateInfo.isAlphaTested = material.alphaMode != "OPAQUE";

            materialCreateInfo.baseColorTextureHandle = material.pbrMetallicRoughness.baseColorTexture.index >= 0
                                                            ? textureHandles[material.pbrMetallicRoughness.baseColorTexture.index]
                                                            : INVALID_TEXTURE_HANDLE;

            materialCreateInfo.metallicRoughnessTextureHandle = material.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0
                                                                    ? textureHandles[material.pbrMetallicRoughness.
                                                                        metallicRoughnessTexture.index]
                                                                    : INVALID_TEXTURE_HANDLE;

            materialCreateInfo.normalMapTextureHandle = material.normalTexture.index >= 0
                                                            ? textureHandles[material.normalTexture.index]
                                                            : INVALID_TEXTURE_HANDLE;

            materials.push_back(device->CreateMaterial(materialCreateInfo));
        }
        return materials;
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
        const tinygltf::Node node = model.nodes[scene.nodes[0]];

        const std::vector<MaterialHandle> materials = LoadMaterials(device, model, gltfFilePath.parent_path());

        auto mesh = CreateRef<VulkanMesh>(device);
        Utils::LoadGLTFNode(node, model, mesh, materials);

        return mesh;
    }

    static void AddNode(VulkanDevice* device,
                        SceneGraph& scene,
                        const SceneNodeHandle parent,
                        const tinygltf::Model& model,
                        const tinygltf::Node& glTFNode,
                        const int level)
    {
        const int currentNodeIndex = scene.nodes.size();
        // Setup node
        {
            scene.nodes.push_back({.parent = parent});
            scene.nodes[currentNodeIndex].handle = currentNodeIndex;
            scene.nodes[currentNodeIndex].level = level;

            scene.names.push_back(glTFNode.name);

            if (parent != INVALID_SCENE_NODE_HANDLE)
            {
                const int64_t firstChildIndex = scene.nodes[parent].firstChild;

                if (firstChildIndex == INVALID_SCENE_NODE_HANDLE)
                {
                    scene.nodes[parent].firstChild = currentNodeIndex;
                    scene.nodes[currentNodeIndex].lastSibling = currentNodeIndex;
                } else
                {
                    int64_t lastSiblingIndex = scene.nodes[firstChildIndex].lastSibling;
                    if (lastSiblingIndex == INVALID_SCENE_NODE_HANDLE)
                    {
                        for (lastSiblingIndex = firstChildIndex;
                             scene.nodes[lastSiblingIndex].nextSibling != INVALID_SCENE_NODE_HANDLE;
                             lastSiblingIndex = scene.nodes[lastSiblingIndex].nextSibling
                        );
                    }
                    scene.nodes[lastSiblingIndex].nextSibling = currentNodeIndex;
                    scene.nodes[firstChildIndex].lastSibling = currentNodeIndex;
                }
            }
        }

        // Load transforms
        {
            Transform transform;
            // Generate local node matrix
            if (glTFNode.translation.size() == 3)
            {
                transform.m_Position = glm::make_vec3(glTFNode.translation.data());
            }
            if (glTFNode.rotation.size() == 4)
            {
                glm::quat quaternion = glm::make_quat(glTFNode.rotation.data());
                transform.m_Rotation = eulerAngles(quaternion) * 180.f / static_cast<float>(M_PI);
            }
            if (glTFNode.scale.size() == 3)
            {
                transform.m_Scale = glm::make_vec3(glTFNode.scale.data());
            }

            scene.transforms.push_back(transform);
        }

        // Load mesh
        {
            const auto mesh = new VulkanMesh(device);
            if (glTFNode.mesh > -1) {
                scene.meshes.push_back(mesh);
                for (auto& primitive: model.meshes[glTFNode.mesh].primitives)
                {
                    Utils::glTFPrimitive meshPrimitive = Utils::LoadPrimitive(primitive, model);
                    mesh->AddMeshlet(meshPrimitive.vertices, meshPrimitive.indices, scene.materials[meshPrimitive.materialIndex]);
                }

            }
        }

        // Load child nodes
        {
            for (const auto& childNode: glTFNode.children)
            {
                AddNode(device, scene, currentNodeIndex, model, model.nodes[childNode], level + 1);
            }
        }
    }

    SceneGraph* GLTFLoader::LoadSceneGraph(VulkanDevice* device, const std::string& scenePath)
    {
        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err, warn;

        const std::filesystem::path gltfFilePath(scenePath);
        const bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, scenePath);

        if (!warn.empty())
            LOG_WARN("Warn: " + warn);
        if (!err.empty())
            LOG_ERROR("Err: " + err);

        if (!ret)
        {
            LOG_ERROR("Failed to parse glTF");
            abort();
        }

        const tinygltf::Scene& gltfScene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];

        const auto sceneGraph = new SceneGraph();
        sceneGraph->materials = LoadMaterials(device, model, gltfFilePath.parent_path());

        for (auto& node: gltfScene.nodes)
            AddNode(device, *sceneGraph, 0, model, model.nodes[node], 1);

        return sceneGraph;
    }
}
