#pragma once
#include <string>
#include <vulkan/vulkan_core.h>

namespace Raytracing
{
	class VulkanPipeline
	{
	public:
		VulkanPipeline(const VkDevice device,
		               const VkRenderPass renderPass,
		               const VkDescriptorSetLayout descriptorSetLayout,
		               const std::string& vertexShaderPath,
		               const std::string& fragmentShaderPath);

		~VulkanPipeline();

		VkPipeline GetPipeline() const { return pipeline; }
		VkPipelineLayout GetPipelineLayout() const { return pipelineLayout; }

	private:
		void CreateGraphicsPipeline(
			const VkRenderPass renderPass,
			const VkDescriptorSetLayout descriptorSetLayout,
			const std::string& vertexShaderPath,
			const std::string& fragmentShaderPath);

	private:
		VkDevice device;
		VkPipeline pipeline;
		VkPipelineLayout pipelineLayout;
	};
}
