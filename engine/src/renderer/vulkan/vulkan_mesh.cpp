#include "renderer/vulkan/vulkan_mesh.h"
#include "renderer/vulkan/vulkan_device.h"

namespace MongooseVK
{
    VulkanMesh::VulkanMesh(VulkanDevice* vulkanDevice): vulkanDevice(vulkanDevice) {}

    VulkanMesh::~VulkanMesh()
    {
        for (const auto& meshlet: meshlets)
        {
            vulkanDevice->DestroyBuffer(meshlet.vertexBuffer);
            vulkanDevice->DestroyBuffer(meshlet.indexBuffer);
        }
    }

    VulkanMeshlet VulkanMesh::MakeMeshlet(VulkanDevice* device, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices,
        MaterialHandle materialHandle)
    {
        return {
            .vertices = vertices,
            .indices = indices,
            .vertexBuffer = CreateVertexBuffer(device, vertices),
            .indexBuffer = CreateIndexBuffer(device, indices),
            .material = materialHandle
        };
    }

    void VulkanMesh::AddMeshlet(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices,
                                MaterialHandle materialHandle)
    {
        const VulkanMeshlet meshlet = {
            .vertices = vertices,
            .indices = indices,
            .vertexBuffer = CreateVertexBuffer(vulkanDevice, vertices),
            .indexBuffer = CreateIndexBuffer(vulkanDevice, indices),
            .material = materialHandle
        };

        meshlets.push_back(meshlet);
    }

    AllocatedBuffer VulkanMesh::CreateVertexBuffer(VulkanDevice* device, const std::vector<Vertex>& vertices)
    {
        ASSERT(vertices.size() >= 3, "Vertex count must be at least 3");

        const auto bufferSize = sizeof(vertices[0]) * vertices.size();
        const auto stagingBuffer = device->CreateBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_CPU_ONLY);

        memcpy(stagingBuffer.GetData(), vertices.data(), bufferSize);

        AllocatedBuffer vertexBuffer = device->CreateBuffer(
            stagingBuffer.GetBufferSize(),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);

        device->CopyBuffer(stagingBuffer, vertexBuffer);
        device->DestroyBuffer(stagingBuffer);

        return vertexBuffer;
    }

    AllocatedBuffer VulkanMesh::CreateIndexBuffer(VulkanDevice* device, const std::vector<uint32_t>& indices)
    {
        const auto bufferSize = sizeof(indices[0]) * indices.size();
        const auto stagingBuffer = device->CreateBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_CPU_ONLY);

        memcpy(stagingBuffer.GetData(), indices.data(), bufferSize);

        AllocatedBuffer indexBuffer = device->CreateBuffer(
            stagingBuffer.GetBufferSize(),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);

        device->CopyBuffer(stagingBuffer, indexBuffer);
        device->DestroyBuffer(stagingBuffer);

        return indexBuffer;
    }
}
