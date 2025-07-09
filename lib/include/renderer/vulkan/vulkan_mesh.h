#pragma once

#include "util/core.h"
#include "renderer/mesh.h"
#include "vulkan_buffer.h"
#include "vulkan_material.h"

namespace MongooseVK
{
    class VulkanDevice;

    class VulkanMeshlet {
    public:
        VulkanMeshlet(VulkanDevice* vulkanDevice, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices,
                      int materialIndex = 0);
        ~VulkanMeshlet() = default;

        VulkanMeshlet(const VulkanMeshlet& otherMeshlet)
        {
            this->vertices = otherMeshlet.vertices;
            this->indices = otherMeshlet.indices;
            this->materialIndex = otherMeshlet.materialIndex;

            vertexBuffer = std::move(otherMeshlet.vertexBuffer);
            indexBuffer = std::move(otherMeshlet.indexBuffer);
        }

        void Bind(VkCommandBuffer commandBuffer) const;

        void CreateVertexBuffer(VulkanDevice* vulkanDevice);
        void CreateIndexBuffer(VulkanDevice* vulkanDevice);

        uint32_t GetIndexCount() const { return indices.size(); }

        void SetMaterialIndex(int _materialIndex) { materialIndex = _materialIndex; }
        int GetMaterialIndex() const { return materialIndex; }

    public:
        VulkanDevice* vulkanDevice;

        int materialIndex = -1;

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        AllocatedBuffer vertexBuffer;
        AllocatedBuffer indexBuffer;
    };

    class VulkanMesh {
    public:
        VulkanMesh(VulkanDevice* vulkanDevice): vulkanDevice(vulkanDevice) {}
        ~VulkanMesh();

        void AddMeshlet(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, int materialIndex);
        std::vector<VulkanMeshlet>& GetMeshlets() { return meshlets; }

        void SetMaterials(const std::vector<VulkanMaterial>& _materials) { materials = _materials; }
        std::vector<VulkanMaterial>& GetMaterials() { return materials; }

        VulkanMaterial& GetMaterial(uint32_t index) { return materials[index]; }
        VulkanMaterial& GetMaterial(VulkanMeshlet& meshlet) { return materials[meshlet.materialIndex]; }

    private:
        VulkanDevice* vulkanDevice;
        std::vector<VulkanMeshlet> meshlets;
        std::vector<VulkanMaterial> materials;
    };
}
