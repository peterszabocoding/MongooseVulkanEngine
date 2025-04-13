#include "vulkan_mesh.h"
#include "vulkan_device.h"

namespace Raytracing
{
    VulkanMesh::VulkanMesh(VulkanDevice* device, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices): Mesh(
        vertices, indices)
    {
        vulkanDevice = device;
        CreateVertexBuffer();
        CreateIndexBuffer();
    }

    void VulkanMesh::CreateVertexBuffer()
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
        
        vertexBuffer = CreateScope<VulkanBuffer>(vulkanDevice,
                                                 bufferSize,
                                                 vertexBufferUsageBits,
                                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                 VMA_MEMORY_USAGE_GPU_ONLY);

        VulkanBuffer::CopyBuffer(vulkanDevice, &stagingBuffer, vertexBuffer.get());
    }

    void VulkanMesh::CreateIndexBuffer()
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

        indexBuffer = CreateScope<VulkanBuffer>(vulkanDevice,
                                                bufferSize,
                                                indexBufferUsageBits,
                                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                VMA_MEMORY_USAGE_GPU_ONLY);

        VulkanBuffer::CopyBuffer(vulkanDevice, &stagingBuffer, indexBuffer.get());
    }

    void VulkanMesh::Bind(VkCommandBuffer commandBuffer) const
    {
        const VkBuffer buffers[] = {vertexBuffer->GetBuffer()};
        const VkDeviceSize offsets[] = {0};

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
    }
}
