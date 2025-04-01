#pragma once

#include <vulkan/vulkan_core.h>
#include <vector>

namespace Raytracing
{
    class VulkanDevice;
    class Vertex;

    class VulkanVertexBuffer {
    public:
        VulkanVertexBuffer(VulkanDevice* device, const std::vector<Vertex>& vertices);
        ~VulkanVertexBuffer();

        void Bind(VkCommandBuffer commandBuffer) const;

    private:
        void CreateVertexBuffer();

    private:
        VulkanDevice* vulkanDevice;
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;

        std::vector<Vertex> vertices;
    };
}
