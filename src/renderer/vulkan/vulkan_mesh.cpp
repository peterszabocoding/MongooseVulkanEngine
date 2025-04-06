#include "vulkan_mesh.h"
#include "vulkan_device.h"

namespace Raytracing
{
    VulkanMesh::VulkanMesh(VulkanDevice* device, const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices): Mesh(
        vertices, indices)
    {
        vulkanDevice = device;
        CreateVertexBuffer();
        CreateIndexBuffer();
    }

    VulkanMesh::~VulkanMesh()
    {
        delete vertexBuffer;
        delete indexBuffer;
    }

    void VulkanMesh::CreateVertexBuffer()
    {
        assert(vertices.size() >= 3 && "Vertex count must be at least 3");

        const auto stagingBuffer = VulkanBuffer(
            vulkanDevice,
            sizeof(vertices[0]) * vertices.size(),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            VMA_MEMORY_USAGE_CPU_ONLY);

        void* data;
        vkMapMemory(vulkanDevice->GetDevice(), stagingBuffer.GetBufferMemory(), 0, stagingBuffer.GetBufferSize(), 0, &data);
        memcpy(data, vertices.data(), stagingBuffer.GetBufferSize());
        vkUnmapMemory(vulkanDevice->GetDevice(), stagingBuffer.GetBufferMemory());

        vertexBuffer = new VulkanBuffer(
            vulkanDevice,
            stagingBuffer.GetBufferSize(),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);

        VulkanBuffer::CopyBuffer(vulkanDevice, vulkanDevice->GetGraphicsQueue(), &stagingBuffer, vertexBuffer);
    }

    void VulkanMesh::CreateIndexBuffer()
    {
        const auto stagingBuffer = VulkanBuffer(
            vulkanDevice,
            sizeof(indices[0]) * indices.size(),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            VMA_MEMORY_USAGE_CPU_ONLY);

        void* data;
        vkMapMemory(vulkanDevice->GetDevice(), stagingBuffer.GetBufferMemory(), 0, stagingBuffer.GetBufferSize(), 0, &data);
        memcpy(data, indices.data(), stagingBuffer.GetBufferSize());
        vkUnmapMemory(vulkanDevice->GetDevice(), stagingBuffer.GetBufferMemory());

        indexBuffer = new VulkanBuffer(
            vulkanDevice,
            sizeof(indices[0]) * indices.size(),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);

        VulkanBuffer::CopyBuffer(vulkanDevice, vulkanDevice->GetGraphicsQueue(), &stagingBuffer, indexBuffer);
    }

    void VulkanMesh::Bind(VkCommandBuffer commandBuffer) const
    {
        const VkBuffer buffers[] = {vertexBuffer->GetBuffer()};
        const VkDeviceSize offsets[] = {0};

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT16);
    }
}
