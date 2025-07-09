#include "renderer/vulkan/vulkan_buffer.h"

#include "renderer/vulkan/vulkan_device.h"
#include "renderer/vulkan/vulkan_utils.h"
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
