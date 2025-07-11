#pragma once

#include <vma/vk_mem_alloc.h>
#include "vulkan_device.h"

namespace MongooseVK
{
    class VulkanDevice;

    class VulkanBuffer {
    public:
        VulkanBuffer(VulkanDevice* vulkanDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                     VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO);
        ~VulkanBuffer();

        VkBuffer& GetBuffer() { return allocatedBuffer.buffer; }
        VkDeviceMemory GetBufferMemory() const { return allocatedBuffer.GetBufferMemory(); }
        VkDeviceSize GetBufferSize() const { return allocatedBuffer.GetBufferSize(); }
        VkDeviceAddress GetBufferAddress() const { return allocatedBuffer.address; }
        VkDeviceSize GetOffset() const { return allocatedBuffer.GetOffset(); }
        void* GetData() const { return allocatedBuffer.GetData(); }

    private:
        VulkanDevice* vulkanDevice;
        AllocatedBuffer allocatedBuffer{};
    };
}
