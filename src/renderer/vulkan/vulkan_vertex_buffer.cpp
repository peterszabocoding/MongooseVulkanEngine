#include "vulkan_vertex_buffer.h"

#include <cassert>
#include "vulkan_utils.h"

Raytracing::VulkanVertexBuffer::VulkanVertexBuffer(
	VkDevice device,
	VkPhysicalDevice physicalDevice, 
	VkCommandPool commandPool,
	VkQueue queue,
	const std::vector<Vertex>& vertices)
{
	this->device = device;
	this->physicalDevice = physicalDevice;
	CreateVertexBuffers(commandPool, queue, vertices);
}

Raytracing::VulkanVertexBuffer::~VulkanVertexBuffer()
{
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);
}

void Raytracing::VulkanVertexBuffer::Bind(VkCommandBuffer commandBuffer) 
{
	VkBuffer buffers[] = { vertexBuffer };
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
}

void Raytracing::VulkanVertexBuffer::Draw(VkCommandBuffer commandBuffer) {}

void Raytracing::VulkanVertexBuffer::CreateVertexBuffers(
	VkCommandPool commandPool,
	VkQueue queue, 
	const std::vector<Vertex>& vertices)
{
	vertexCount = static_cast<uint32_t>(vertices.size());
	assert(vertexCount >= 3 && "Vertex count must be at least 3");

	const VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
		VkBuffer staging_buffer;
		VkDeviceMemory staging_buffer_memory;
		VulkanUtils::CreateBuffer(
			device,
			physicalDevice,
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			staging_buffer,
			staging_buffer_memory);

		void* data;
		vkMapMemory(device, staging_buffer_memory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), bufferSize);
		vkUnmapMemory(device, staging_buffer_memory);

		VulkanUtils::CreateBuffer(
			device,
			physicalDevice,
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			vertexBuffer, 
			vertexBufferMemory);

		VulkanUtils::CopyBuffer(device, commandPool, queue, staging_buffer, vertexBuffer, bufferSize);

		vkDestroyBuffer(device, staging_buffer, nullptr);
		vkFreeMemory(device, staging_buffer_memory, nullptr);
}
