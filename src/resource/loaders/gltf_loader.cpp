#include "gltf_loader.h"

#include <glm/gtc/type_ptr.hpp>
#include <tiny_gltf/tiny_gltf.h>

#include "renderer/mesh.h"
#include "renderer/vulkan/vulkan_mesh.h"
#include "util/log.h"

namespace Raytracing
{

    namespace Utils
    {
        static void LoadGLTFNode(const tinygltf::Node& node, const tinygltf::Model& model, std::vector<Vertex>& vertices,std::vector<uint32_t>& indices)
        {
            /*
        vkglTF::Node *newNode = new Node{};
        newNode->index = nodeIndex;
        newNode->parent = parent;
        newNode->name = node.name;
        newNode->skinIndex = node.skin;
        newNode->matrix = glm::mat4(1.0f);

        // Generate local node matrix
        glm::vec3 translation = glm::vec3(0.0f);
        if (node.translation.size() == 3) {
            translation = glm::make_vec3(node.translation.data());
            newNode->translation = translation;
        }
        glm::mat4 rotation = glm::mat4(1.0f);
        if (node.rotation.size() == 4) {
            glm::quat q = glm::make_quat(node.rotation.data());
            newNode->rotation = glm::mat4(q);
        }
        glm::vec3 scale = glm::vec3(1.0f);
        if (node.scale.size() == 3) {
            scale = glm::make_vec3(node.scale.data());
            newNode->scale = scale;
        }
        if (node.matrix.size() == 16) {
            newNode->matrix = glm::make_mat4x4(node.matrix.data());
        };

        // Node with children
        if (node.children.size() > 0) {
            for (size_t i = 0; i < node.children.size(); i++) {
                loadNode(newNode, model.nodes[node.children[i]], node.children[i], model, loaderInfo, globalscale);
            }
        }
        */

        // Node contains mesh data
        if (node.mesh > -1)
        {
            const tinygltf::Mesh mesh = model.meshes[node.mesh];

            for (size_t j = 0; j < mesh.primitives.size(); j++)
            {
                const tinygltf::Primitive& primitive = mesh.primitives[j];
                uint32_t vertexPos = 0;
                uint32_t indexPos = 0;
                uint32_t indexCount = 0;
                uint32_t vertexCount = 0;
                glm::vec3 posMin{};
                glm::vec3 posMax{};

                bool hasIndices = primitive.indices > -1;
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
                        vert.normal = glm::normalize(
                            glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * normByteStride]) : glm::vec3(0.0f)));
                        vert.texCoord = bufferTexCoordSet0 ? glm::make_vec2(&bufferTexCoordSet0[v * uv0ByteStride]) : glm::vec3(0.0f);
                        vert.color = bufferColorSet0 ? glm::make_vec4(&bufferColorSet0[v * color0ByteStride]) : glm::vec4(1.0f);

                        vertexPos++;
                    }
                }

                // Indices
                if (hasIndices)
                {
                    const tinygltf::Accessor& accessor = model.accessors[primitive.indices > -1 ? primitive.indices : 0];
                    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

                    indexCount = static_cast<uint32_t>(accessor.count);
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

                /*
                Primitive* newPrimitive = new Primitive(indexStart, indexCount, vertexCount,
                                                        primitive.material > -1 ? materials[primitive.material] : materials.back());
                newPrimitive->setBoundingBox(posMin, posMax);
                newMesh->primitives.push_back(newPrimitive);
*/
            }
            /*
            // Mesh BB from BBs of primitives
            for (auto p: newMesh->primitives)
            {
                if (p->bb.valid && !newMesh->bb.valid)
                {
                    newMesh->bb = p->bb;
                    newMesh->bb.valid = true;
                }
                newMesh->bb.min = glm::min(newMesh->bb.min, p->bb.min);
                newMesh->bb.max = glm::max(newMesh->bb.max, p->bb.max);
            }
            newNode->mesh = newMesh;
            */
        }
        /*
        if (parent)
        {
            parent->children.push_back(newNode);
        } else
        {
            nodes.push_back(newNode);
        }
        linearNodes.push_back(newNode);
        */
        }

        static void GetNodeProps(const tinygltf::Node& node, const tinygltf::Model& model, size_t& vertexCount, size_t& indexCount)
        {
            if (node.children.size() > 0)
            {
                for (size_t i = 0; i < node.children.size(); i++)
                {
                    GetNodeProps(model.nodes[node.children[i]], model, vertexCount, indexCount);
                }
            }
            if (node.mesh > -1)
            {
                const tinygltf::Mesh mesh = model.meshes[node.mesh];
                for (size_t i = 0; i < mesh.primitives.size(); i++)
                {
                    auto primitive = mesh.primitives[i];
                    vertexCount += model.accessors[primitive.attributes.find("POSITION")->second].count;
                    if (primitive.indices > -1)
                    {
                        indexCount += model.accessors[primitive.indices].count;
                    }
                }
            }
        }
    }

    Ref<VulkanMesh> GLTFLoader::LoadMesh(VulkanDevice* device, const std::string& meshPath)
    {
        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err;
        std::string warn;

        const bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, meshPath);

        if (!warn.empty()) LOG_TRACE("Warn: " + warn);
        if (!err.empty()) LOG_TRACE("Err: " + err);

        if (!ret)
        {
            LOG_TRACE("Failed to parse glTF");
            abort();
        }

        size_t vertexCount = 0;
        size_t indexCount = 0;

        const tinygltf::Scene& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];

        for (size_t i = 0; i < scene.nodes.size(); i++)
        {
            Utils::GetNodeProps(model.nodes[scene.nodes[i]], model, vertexCount, indexCount);
        }

        std::vector<Vertex> vertices(vertexCount);
        std::vector<uint32_t> indices(indexCount);

        for (size_t i = 0; i < scene.nodes.size(); i++)
        {
            const tinygltf::Node node = model.nodes[scene.nodes[i]];
            Utils::LoadGLTFNode(node, model, vertices, indices);
        }

        return CreateRef<VulkanMesh>(device, vertices, indices);
    }
}
