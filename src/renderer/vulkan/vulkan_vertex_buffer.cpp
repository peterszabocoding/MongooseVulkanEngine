#include "vulkan_vertex_buffer.h"

#include <cassert>
#include "vulkan_utils.h"
#include "vulkan_device.h"

#include "renderer/mesh.h"

namespace Raytracing
{
    VulkanVertexBuffer::VulkanVertexBuffer(VulkanDevice* device, const std::vector<Vertex>& vertices)
    {
        vulkanDevice = device;
        this->vertices = vertices;
        CreateVertexBuffer();
    }

    VulkanVertexBuffer::~VulkanVertexBuffer()
    {
        delete vertexBuffer;
    }

    void VulkanVertexBuffer::Bind(const VkCommandBuffer commandBuffer) const
    {
        const VkBuffer buffers[] = {vertexBuffer->GetBuffer()};
        const VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
    }

    void VulkanVertexBuffer::CreateVertexBuffer()
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
}
