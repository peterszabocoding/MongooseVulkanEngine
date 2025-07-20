#include "renderer/vulkan/vulkan_buffer.h"

#include "renderer/vulkan/vulkan_device.h"

namespace MongooseVK
{
    VulkanBuffer::VulkanBuffer(VulkanDevice* _vulkanDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                               VmaMemoryUsage memoryUsage)
    {
        vulkanDevice = _vulkanDevice;
        allocatedBuffer = vulkanDevice->CreateBuffer(size, usage, memoryUsage);
    }

    VulkanBuffer::~VulkanBuffer()
    {
        vulkanDevice->DestroyBuffer(allocatedBuffer);
    }
}
