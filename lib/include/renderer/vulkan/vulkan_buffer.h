#pragma once

#include <vma/vk_mem_alloc.h>
#include "vulkan_device.h"

namespace Raytracing
{
    class VulkanDevice;

    class VulkanBuffer {
    public:
        VulkanBuffer(VulkanDevice* vulkanDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                     VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO);
        ~VulkanBuffer();

        VkBuffer& GetBuffer() { return allocatedBuffer.buffer; }
        VkDeviceMemory GetBufferMemory() const { return allocatedBuffer.info.deviceMemory; }
        VkDeviceSize GetBufferSize() const { return allocatedBuffer.info.size; }
        VkDeviceAddress GetBufferAddress() const { return allocatedBuffer.address; }
        VkDeviceSize GetOffset() const { return allocatedBuffer.info.offset; }
        void* GetData() const { return allocatedBuffer.info.pMappedData; }

    private:
        VulkanDevice* vulkanDevice;
        AllocatedBuffer allocatedBuffer{};
    };
}
