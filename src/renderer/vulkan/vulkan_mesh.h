#pragma once

#include "renderer/mesh.h"
#include "vulkan_buffer.h"

namespace Raytracing
{
    class VulkanDevice;

    class VulkanMesh : public Mesh {
    public:
        VulkanMesh(VulkanDevice* vulkanDevice, const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices);
        ~VulkanMesh() override;

        void Bind(VkCommandBuffer commandBuffer) const;

    private:
        void CreateVertexBuffer();
        void CreateIndexBuffer();

    private:
        VulkanDevice* vulkanDevice;
        VulkanBuffer* vertexBuffer;
        VulkanBuffer* indexBuffer;
    };
}
