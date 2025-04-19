#pragma once

#include "renderer/mesh.h"
#include "vulkan_buffer.h"
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

            vertexBuffer = std::move(otherMeshlet.vertexBuffer);
            indexBuffer = std::move(otherMeshlet.indexBuffer);
        }

        VulkanMeshlet& operator=(const VulkanMeshlet&) = delete;

        void Bind(VkCommandBuffer commandBuffer) const;

        void CreateVertexBuffer(VulkanDevice* vulkanDevice);
        void CreateIndexBuffer(VulkanDevice* vulkanDevice);

        uint32_t GetIndexCount() const { return indices.size(); }

    public:
        int materialIndex = -1;

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        Ref<VulkanBuffer> vertexBuffer;
        Ref<VulkanBuffer> indexBuffer;
    };

    class VulkanMesh {
    public:
        VulkanMesh(VulkanDevice* vulkanDevice, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
        ~VulkanMesh() = default;

        void SetMaterialIndex(int materialIndex) { this->materialIndex = materialIndex; }

        const std::vector<VulkanMeshlet>& GetMeshlets() const { return meshlets; }
        int GetMaterialIndex() const { return materialIndex; }

    private:
        VulkanDevice* vulkanDevice;
        std::vector<VulkanMeshlet> meshlets;
        int materialIndex = 0;
    };
}
