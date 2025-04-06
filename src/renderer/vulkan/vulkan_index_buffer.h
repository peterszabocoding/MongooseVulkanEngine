#pragma once
#include "vulkan_buffer.h"
#include "vulkan_device.h"

namespace Raytracing
{
    class VulkanIndexBuffer {
    public:
        VulkanIndexBuffer(VulkanDevice* device, const std::vector<uint16_t>& mesh_indices);
        ~VulkanIndexBuffer();

        void Bind(VkCommandBuffer commandBuffer) const;
        uint32_t GetIndexCount() const;

    private:
        void CreateIndexBuffer();

    private:
        VulkanDevice* vulkanDevice;
        VulkanBuffer* indexBuffer;

        std::vector<uint16_t> indices;
    };
}
