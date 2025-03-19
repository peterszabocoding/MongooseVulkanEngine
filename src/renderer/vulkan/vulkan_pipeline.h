#pragma once
#include <string>
#include <vulkan/vulkan_core.h>

namespace Raytracing
{
	class VulkanDevice;

	class VulkanPipeline
	{
	public:
		VulkanPipeline(VulkanDevice* vulkanDevice,
		               const std::string& vertexShaderPath,
		               const std::string& fragmentShaderPath);

		~VulkanPipeline();

		VkPipeline GetPipeline() const { return pipeline; }
		VkPipelineLayout GetPipelineLayout() const { return pipelineLayout; }
		VkDescriptorSetLayout GetDescriptorSetLayout() const { return descriptorSetLayout; }

	private:
		void CreateDescriptorSetLayout();
		void CreateGraphicsPipeline(
			const VkRenderPass renderPass,
			const std::string& vertexShaderPath,
			const std::string& fragmentShaderPath);

	private:
		VulkanDevice* vulkanDevice;
		VkPipeline pipeline;
		VkPipelineLayout pipelineLayout;

		VkDescriptorSetLayout descriptorSetLayout;
	};
}
