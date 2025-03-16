#pragma once
#include "vulkan_device.h"

namespace Raytracing
{
	class VulkanIndexBuffer
	{
	public:
		VulkanIndexBuffer(
			VulkanDevice* device,
			const std::vector<uint16_t>& mesh_indices);
		~VulkanIndexBuffer();

		void Bind(VkCommandBuffer commandBuffer) const;
		static void Draw(VkCommandBuffer commandBuffer);

	private:
		void CreateIndexBuffer(const std::vector<uint16_t>& mesh_indices);

	private:
		VulkanDevice* vulkanDevice;
		VkBuffer indexBuffer;
		VkDeviceMemory indexBufferMemory;
	};
}
