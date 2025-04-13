#pragma once

#include "renderer/mesh.h"
#include "vulkan_buffer.h"
#include "util/core.h"

namespace Raytracing
{
    class VulkanDevice;

    class VulkanMesh : public Mesh {
    public:
        VulkanMesh(VulkanDevice* vulkanDevice, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
        ~VulkanMesh() override = default;

        void Bind(VkCommandBuffer commandBuffer) const;

    private:
        void CreateVertexBuffer();
        void CreateIndexBuffer();

    private:
        VulkanDevice* vulkanDevice;
        Scope<VulkanBuffer> vertexBuffer;
        Scope<VulkanBuffer> indexBuffer;
    };
}
