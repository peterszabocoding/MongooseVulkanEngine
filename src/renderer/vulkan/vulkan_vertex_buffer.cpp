#include "vulkan_vertex_buffer.h"

#include <cassert>

Raytracing::VulkanVertexBuffer::VulkanVertexBuffer(VkDevice device, const std::vector<Vertex>& vertices)
{
	this->device = device;
	CreateVertexBuffers(vertices);
}

Raytracing::VulkanVertexBuffer::~VulkanVertexBuffer()
{
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);
}

void Raytracing::VulkanVertexBuffer::Bind(VkCommandBuffer commandBuffer) {}

void Raytracing::VulkanVertexBuffer::Draw(VkCommandBuffer commandBuffer) {}

void Raytracing::VulkanVertexBuffer::CreateVertexBuffers(const std::vector<Vertex>& vertices)
{
	vertexCount = static_cast<uint32_t>(vertices.size());
	assert(vertexCount >= 3 && "Vertex count must be at least 3");
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
}
