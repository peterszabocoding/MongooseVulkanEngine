#include "vulkan_index_buffer.h"

#include "vulkan_buffer.h"
#include "vulkan_utils.h"
#include "vulkan_device.h"

namespace Raytracing
{
    VulkanIndexBuffer::VulkanIndexBuffer(VulkanDevice* device, const std::vector<uint16_t>& mesh_indices)
    {
        vulkanDevice = device;
        indices = mesh_indices;
        CreateIndexBuffer();
    }

    VulkanIndexBuffer::~VulkanIndexBuffer()
    {
        delete indexBuffer;
    }

    void VulkanIndexBuffer::Bind(VkCommandBuffer commandBuffer) const
    {
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT16);
    }

    uint32_t VulkanIndexBuffer::GetIndexCount() const
    {
        return indices.size();
    }

    void VulkanIndexBuffer::CreateIndexBuffer()
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
}
