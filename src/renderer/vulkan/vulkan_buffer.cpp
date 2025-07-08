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
        allocatedBuffer = vulkanDevice->AllocateBuffer(size, usage, memoryUsage);
    }

    VulkanBuffer::~VulkanBuffer()
    {
        vulkanDevice->FreeBuffer(allocatedBuffer);
    }
}
