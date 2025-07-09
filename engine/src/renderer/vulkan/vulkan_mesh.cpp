#include "renderer/vulkan/vulkan_mesh.h"
#include "renderer/vulkan/vulkan_device.h"

namespace MongooseVK
{
    VulkanMeshlet::VulkanMeshlet(VulkanDevice* _vulkanDevice, const std::vector<Vertex>& _vertices, const std::vector<uint32_t>& _indices,
                                 int _materialIndex)
    {
        vulkanDevice = _vulkanDevice;
        vertices = _vertices;
        indices = _indices;
        materialIndex = _materialIndex;

        CreateVertexBuffer(vulkanDevice);
        CreateIndexBuffer(vulkanDevice);
    }

    void VulkanMeshlet::Bind(VkCommandBuffer commandBuffer) const
    {
        const VkBuffer buffers[] = {vertexBuffer.buffer};
        const VkDeviceSize offsets[] = {0};

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    }

    void VulkanMeshlet::CreateVertexBuffer(VulkanDevice* vulkanDevice)
    {
        assert(vertices.size() >= 3 && "Vertex count must be at least 3");

        const auto bufferSize = sizeof(vertices[0]) * vertices.size();
        const auto stagingBuffer = vulkanDevice->AllocateBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_CPU_ONLY);

        memcpy(stagingBuffer.GetData(), vertices.data(), bufferSize);

        vertexBuffer = vulkanDevice->AllocateBuffer(
            stagingBuffer.GetBufferSize(),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);

        vulkanDevice->CopyBuffer(stagingBuffer, vertexBuffer);
        vulkanDevice->FreeBuffer(stagingBuffer);
    }

    void VulkanMeshlet::CreateIndexBuffer(VulkanDevice* vulkanDevice)
    {
        const auto bufferSize = sizeof(indices[0]) * indices.size();
        const auto stagingBuffer = vulkanDevice->AllocateBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_CPU_ONLY);

        memcpy(stagingBuffer.GetData(), indices.data(), bufferSize);

        indexBuffer = vulkanDevice->AllocateBuffer(
            stagingBuffer.GetBufferSize(),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);

        vulkanDevice->CopyBuffer(stagingBuffer, indexBuffer);
        vulkanDevice->FreeBuffer(stagingBuffer);
    }

    VulkanMesh::~VulkanMesh()
    {
        for (const auto& meshlet: meshlets)
        {
            vulkanDevice->FreeBuffer(meshlet.vertexBuffer);
            vulkanDevice->FreeBuffer(meshlet.indexBuffer);
        }
    }

    void VulkanMesh::AddMeshlet(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, int materialIndex)
    {
        meshlets.emplace_back(vulkanDevice, vertices, indices, materialIndex);
    }
}
