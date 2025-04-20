#pragma once

#include "renderer/mesh.h"
#include "vulkan_buffer.h"
#include "vulkan_material.h"
#include "util/core.h"

namespace Raytracing
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

        VulkanMeshlet& operator=(const VulkanMeshlet&) = delete;

        void Bind(VkCommandBuffer commandBuffer) const;

        void CreateVertexBuffer(VulkanDevice* vulkanDevice);
        void CreateIndexBuffer(VulkanDevice* vulkanDevice);

        uint32_t GetIndexCount() const { return indices.size(); }

        void SetMaterialIndex(int materialIndex) { this->materialIndex = materialIndex; }
        int GetMaterialIndex() const { return materialIndex; }

    public:
        int materialIndex = -1;

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        Ref<VulkanBuffer> vertexBuffer;
        Ref<VulkanBuffer> indexBuffer;
    };

    class VulkanMesh {
    public:
        VulkanMesh(VulkanDevice* vulkanDevice): vulkanDevice(vulkanDevice) {}
        ~VulkanMesh() = default;

        void AddMeshlet(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, int materialIndex);
        const std::vector<VulkanMeshlet>& GetMeshlets() const { return meshlets; }

        void SetMaterials(const std::vector<VulkanMaterial>& materials) { this->materials = materials; }
        const std::vector<VulkanMaterial>& GetMaterials() const { return materials; }

    private:
        VulkanDevice* vulkanDevice;
        std::vector<VulkanMeshlet> meshlets;
        std::vector<VulkanMaterial> materials;
    };
}
