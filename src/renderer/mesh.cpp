#include "mesh.h"

namespace Raytracing
{
    Mesh::Mesh(VulkanDevice* device, const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices)
    {
        vertexBuffer = new VulkanVertexBuffer(device, vertices);
        indexBuffer = new VulkanIndexBuffer(device, indices);
    }

    Mesh::~Mesh()
    {
        delete vertexBuffer;
        delete indexBuffer;
    }

    void Mesh::Bind(VkCommandBuffer commandBuffer) const
    {
        vertexBuffer->Bind(commandBuffer);
        indexBuffer->Bind(commandBuffer);
    }

    uint32_t Mesh::GetIndexCount() const
    {
        return indexBuffer->GetIndexCount();
    }
}
