#include "vulkan_index_buffer.h"

#include "vulkan_utils.h"
#include "vulkan_device.h"

namespace Raytracing
{
	VulkanIndexBuffer::VulkanIndexBuffer(VulkanDevice* device, const std::vector<uint16_t>& mesh_indices)
	{
		vulkanDevice = device;
		CreateIndexBuffer(mesh_indices);
	}

	VulkanIndexBuffer::~VulkanIndexBuffer()
	{
		vkDestroyBuffer(vulkanDevice->GetDevice(), indexBuffer, nullptr);
		vkFreeMemory(vulkanDevice->GetDevice(), indexBufferMemory, nullptr);
	}

	void VulkanIndexBuffer::Bind(VkCommandBuffer commandBuffer) const
	{
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
	}

	void VulkanIndexBuffer::CreateIndexBuffer(const std::vector<uint16_t>& mesh_indices)
	{
		const VkDeviceSize buffer_size = sizeof(mesh_indices[0]) * mesh_indices.size();

		VkBuffer staging_buffer;
		VkDeviceMemory stagingBufferMemory;
		VulkanUtils::CreateBuffer(
			vulkanDevice->GetDevice(),
			vulkanDevice->GetPhysicalDevice(),
			buffer_size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			staging_buffer, stagingBufferMemory);

		void* data;
		vkMapMemory(vulkanDevice->GetDevice(), stagingBufferMemory, 0, buffer_size, 0, &data);
		memcpy(data, mesh_indices.data(), buffer_size);
		vkUnmapMemory(vulkanDevice->GetDevice(), stagingBufferMemory);

		VulkanUtils::CreateBuffer(
			vulkanDevice->GetDevice(),
			vulkanDevice->GetPhysicalDevice(),
			buffer_size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			indexBuffer, indexBufferMemory);

		VulkanUtils::CopyBuffer(vulkanDevice->GetDevice(), vulkanDevice->GetCommandPool(), vulkanDevice->GetGraphicsQueue(), staging_buffer,
		                        indexBuffer, buffer_size);

		vkDestroyBuffer(vulkanDevice->GetDevice(), staging_buffer, nullptr);
		vkFreeMemory(vulkanDevice->GetDevice(), stagingBufferMemory, nullptr);
	}
}
