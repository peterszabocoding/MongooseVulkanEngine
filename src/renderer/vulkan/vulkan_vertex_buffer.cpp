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
        vkDestroyBuffer(vulkanDevice->GetDevice(), vertexBuffer, nullptr);
        vkFreeMemory(vulkanDevice->GetDevice(), vertexBufferMemory, nullptr);
    }

    void VulkanVertexBuffer::Bind(VkCommandBuffer commandBuffer) const
    {
        VkBuffer buffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
    }

    void VulkanVertexBuffer::CreateVertexBuffer()
    {
        assert(vertices.size() >= 3 && "Vertex count must be at least 3");

        const VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        VkBuffer staging_buffer;
        VkDeviceMemory staging_buffer_memory;
        VulkanUtils::CreateBuffer(
            vulkanDevice->GetDevice(),
            vulkanDevice->GetPhysicalDevice(),
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            staging_buffer,
            staging_buffer_memory);

        void* data;
        vkMapMemory(vulkanDevice->GetDevice(), staging_buffer_memory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), bufferSize);
        vkUnmapMemory(vulkanDevice->GetDevice(), staging_buffer_memory);

        VulkanUtils::CreateBuffer(
            vulkanDevice->GetDevice(),
            vulkanDevice->GetPhysicalDevice(),
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            vertexBuffer,
            vertexBufferMemory);

        VulkanUtils::CopyBuffer(
            vulkanDevice->GetDevice(),
            vulkanDevice->GetCommandPool(),
            vulkanDevice->GetGraphicsQueue(),
            staging_buffer,
            vertexBuffer,
            bufferSize);

        vkDestroyBuffer(vulkanDevice->GetDevice(), staging_buffer, nullptr);
        vkFreeMemory(vulkanDevice->GetDevice(), staging_buffer_memory, nullptr);
    }
}
