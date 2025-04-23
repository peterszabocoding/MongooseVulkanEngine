#include "vulkan_mesh.h"
#include "vulkan_device.h"

namespace Raytracing
{
    VulkanMeshlet::VulkanMeshlet(VulkanDevice* vulkanDevice, const std::vector<Vertex>& _vertices, const std::vector<uint32_t>& _indices,
                                 int _materialIndex)
    {
        vertices = _vertices;
        indices = _indices;
        materialIndex = _materialIndex;

        CreateVertexBuffer(vulkanDevice);
        CreateIndexBuffer(vulkanDevice);
    }

    void VulkanMeshlet::Bind(VkCommandBuffer commandBuffer) const
    {
        const VkBuffer buffers[] = {vertexBuffer->GetBuffer()};
        const VkDeviceSize offsets[] = {0};

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
    }

    void VulkanMeshlet::CreateVertexBuffer(VulkanDevice* vulkanDevice)
    {
        assert(vertices.size() >= 3 && "Vertex count must be at least 3");

        auto bufferSize = sizeof(vertices[0]) * vertices.size();

        const auto stagingBuffer = VulkanBuffer(
            vulkanDevice,
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            VMA_MEMORY_USAGE_CPU_ONLY);

        memcpy(stagingBuffer.GetMappedData(), vertices.data(), bufferSize);

        VkBufferUsageFlags vertexBufferUsageBits =
                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        vertexBuffer = CreateRef<VulkanBuffer>(vulkanDevice,
                                               bufferSize,
                                               vertexBufferUsageBits,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                               VMA_MEMORY_USAGE_GPU_ONLY);

        VulkanBuffer::CopyBuffer(vulkanDevice, &stagingBuffer, vertexBuffer.get());
    }

    void VulkanMeshlet::CreateIndexBuffer(VulkanDevice* vulkanDevice)
    {
        auto bufferSize = sizeof(indices[0]) * indices.size();
        const auto stagingBuffer = VulkanBuffer(
            vulkanDevice,
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            VMA_MEMORY_USAGE_CPU_ONLY);

        memcpy(stagingBuffer.GetMappedData(), indices.data(), bufferSize);

        VkBufferUsageFlags indexBufferUsageBits =
                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        indexBuffer = CreateRef<VulkanBuffer>(vulkanDevice,
                                              bufferSize,
                                              indexBufferUsageBits,
                                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                              VMA_MEMORY_USAGE_GPU_ONLY);

        VulkanBuffer::CopyBuffer(vulkanDevice, &stagingBuffer, indexBuffer.get());
    }

    void VulkanMesh::AddMeshlet(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, int materialIndex)
    {
        meshlets.push_back({vulkanDevice, vertices, indices, materialIndex});
    }
}
