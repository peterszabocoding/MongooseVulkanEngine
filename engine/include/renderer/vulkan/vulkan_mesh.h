#pragma once

#include "renderer/mesh.h"
#include "vulkan_buffer.h"
#include "vulkan_material.h"

namespace MongooseVK
{
    class VulkanDevice;

    struct VulkanMeshlet {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        AllocatedBuffer vertexBuffer;
        AllocatedBuffer indexBuffer;

        MaterialHandle material = INVALID_MATERIAL_HANDLE;

        void Bind(const VkCommandBuffer commandBuffer) const
        {
            const VkBuffer buffers[] = {vertexBuffer.buffer};
            const VkDeviceSize offsets[] = {0};

            vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        }
    };

    class VulkanMesh {
    public:
        VulkanMesh(VulkanDevice* vulkanDevice);
        ~VulkanMesh();

        void AddMeshlet(const std::vector<Vertex>& vertices,
                        const std::vector<uint32_t>& indices,
                        MaterialHandle materialHandle = INVALID_MATERIAL_HANDLE);

        std::vector<VulkanMeshlet>& GetMeshlets() { return meshlets; }

    private:
        AllocatedBuffer CreateVertexBuffer(const std::vector<Vertex>& vertices);
        AllocatedBuffer CreateIndexBuffer(const std::vector<uint32_t>& indices);

    private:
        VulkanDevice* vulkanDevice;
        std::vector<VulkanMeshlet> meshlets;
    };
}
