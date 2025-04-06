#include "vulkan_buffer.h"

#include "vulkan_device.h"
#include "vulkan_utils.h"

namespace Raytracing
{
    VulkanBuffer::VulkanBuffer(VulkanDevice* vulkanDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                               VmaMemoryUsage memoryUsage)
    {
        this->vulkanDevice = vulkanDevice;
        CreateBuffer(size, usage, properties, memoryUsage);
    }

    VulkanBuffer::~VulkanBuffer()
    {
        vmaDestroyBuffer(vulkanDevice->GetVmaAllocator(), allocatedBuffer.buffer, allocatedBuffer.allocation);
    }

    void VulkanBuffer::CopyBuffer(const VulkanDevice* vulkanDevice, const VkQueue queue, const VulkanBuffer* src, const VulkanBuffer* dst)
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = vulkanDevice->GetCommandPool();
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(vulkanDevice->GetDevice(), &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkBufferCopy copy_region;
        copy_region.srcOffset = 0; // Optional
        copy_region.dstOffset = 0; // Optional
        copy_region.size = src->GetBufferSize();
        vkCmdCopyBuffer(commandBuffer, src->GetBuffer(), dst->GetBuffer(), 1, &copy_region);

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);

        vkFreeCommandBuffers(vulkanDevice->GetDevice(), vulkanDevice->GetCommandPool(), 1, &commandBuffer);
    }

    void VulkanBuffer::CreateBuffer(const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties,
                                    VmaMemoryUsage memoryUsage)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;

        VmaAllocationCreateInfo vmaallocInfo = {};
        vmaallocInfo.usage = memoryUsage;
        vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VK_CHECK(vmaCreateBuffer(vulkanDevice->GetVmaAllocator(),
            &bufferInfo,
            &vmaallocInfo,
            &allocatedBuffer.buffer,
            &allocatedBuffer.allocation,
            &allocatedBuffer.info));
    }
}
