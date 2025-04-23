#include "vulkan_buffer.h"

#include "vulkan_device.h"
#include "vulkan_utils.h"
#include "util/log.h"

namespace Raytracing
{
    VulkanBuffer::VulkanBuffer(VulkanDevice* _vulkanDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                               VmaMemoryUsage memoryUsage)
    {
        vulkanDevice = _vulkanDevice;
        CreateBuffer(size, usage, memoryUsage);
    }

    VulkanBuffer::~VulkanBuffer()
    {
        vmaDestroyBuffer(vulkanDevice->GetVmaAllocator(), allocatedBuffer.buffer, allocatedBuffer.allocation);
    }

    void VulkanBuffer::CopyBuffer(const VulkanDevice* vulkanDevice, const VulkanBuffer* src, const VulkanBuffer* dst)
    {
        vulkanDevice->ImmediateSubmit([&](VkCommandBuffer commandBuffer) {
            VkBufferCopy copy_region;
            copy_region.srcOffset = 0; // Optional
            copy_region.dstOffset = 0; // Optional
            copy_region.size = src->GetBufferSize();
            vkCmdCopyBuffer(commandBuffer, src->GetBuffer(), dst->GetBuffer(), 1, &copy_region);
        });
    }

    void VulkanBuffer::CreateBuffer(const VkDeviceSize size, const VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
    {
        LOG_TRACE("Allocate buffer: " + std::to_string(size / 1024) + " kB");

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

        VkBufferDeviceAddressInfo deviceAdressInfo{};
        deviceAdressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        deviceAdressInfo.buffer = allocatedBuffer.buffer;

        allocatedBuffer.address = vkGetBufferDeviceAddress(vulkanDevice->GetDevice(), &deviceAdressInfo);
    }
}
