#pragma once

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace Raytracing
{
    class VulkanDevice;

    struct AllocatedBuffer {
        VkBuffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo info;
        VkDeviceAddress address;
    };

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
        void* GetMappedData() const { return allocatedBuffer.info.pMappedData; }

        static void CopyBuffer(const VulkanDevice* vulkanDevice, VulkanBuffer* src, VulkanBuffer* dst);

    private:
        void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

    private:
        VulkanDevice* vulkanDevice;
        AllocatedBuffer allocatedBuffer{};
    };
}
