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

	private:
		void CreateGraphicsPipeline(
			const VkRenderPass renderPass,
			const VkDescriptorSetLayout descriptorSetLayout,
			const std::string& vertexShaderPath,
			const std::string& fragmentShaderPath);

	private:
		VulkanDevice* vulkanDevice;
		VkPipeline pipeline;
		VkPipelineLayout pipelineLayout;
	};
}
