#pragma once
#include <vulkan/vulkan_core.h>
#include <vector>

#include "renderer/mesh.h"

namespace Raytracing
{
	class VulkanVertexBuffer
	{
	public:
		VulkanVertexBuffer(VkDevice device, const std::vector<Vertex>& vertices);
		~VulkanVertexBuffer();

		void Bind(VkCommandBuffer commandBuffer);
		void Draw(VkCommandBuffer commandBuffer);

	private:
		void CreateVertexBuffers(const std::vector<Vertex>& vertices);

	private:
		VkDevice device;
		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		uint32_t vertexCount;
	};
}
