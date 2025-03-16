#pragma once

#include <vulkan/vulkan_core.h>
#include <vector>

#include "renderer/mesh.h"


namespace Raytracing
{
	class VulkanDevice;

	class VulkanVertexBuffer
	{
	public:
		VulkanVertexBuffer(
			VulkanDevice* device,
			const std::vector<Vertex>& vertices);
		~VulkanVertexBuffer();

		void Bind(VkCommandBuffer commandBuffer) const;

	private:
		void CreateVertexBuffer(
			VkCommandPool commandPool,
			VkQueue queue,
			const std::vector<Vertex>& vertices);

	private:
		VulkanDevice* vulkanDevice;
		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		uint32_t vertexCount;
	};
}
