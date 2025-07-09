#include "resource/loaders/obj_loader.h"

#include <vector>
#include <tiny_obj_loader/tiny_obj_loader.h>

#include "renderer/vulkan/vulkan_mesh.h"
#include "renderer/mesh.h"
#include "util/log.h"

namespace MongooseVK
{
    Ref<VulkanMesh> ObjLoader::LoadMesh(VulkanDevice* device, const std::string& meshPath)
    {
        LOG_TRACE("Load Mesh: " + meshPath);

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!LoadObj(&attrib, &shapes, &materials, &warn, &err, meshPath.c_str()))
            throw std::runtime_error(warn + err);

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};
        for (const auto& shape: shapes)
        {
            for (const auto& index: shape.mesh.indices)
            {
                Vertex vertex{};

                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };

                vertex.normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };

                vertex.color = {1.0f, 1.0f, 1.0f};

                if (uniqueVertices.count(vertex) == 0)
                {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }

                indices.push_back(uniqueVertices[vertex]);
            }
        }

        auto mesh = CreateRef<VulkanMesh>(device);
        mesh->AddMeshlet(vertices, indices, 0);

        return mesh;
    }
}
