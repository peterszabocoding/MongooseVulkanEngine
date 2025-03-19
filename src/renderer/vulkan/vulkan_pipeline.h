#pragma once
#include <string>
#include <vulkan/vulkan_core.h>
#include "vulkan_shader.h"

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
		VulkanShader* GetShader() const { return shader; }


	private:
		void CreateGraphicsPipeline(
			const std::string& vertexShaderPath,
			const std::string& fragmentShaderPath);

	private:
		VulkanDevice* vulkanDevice;
		VulkanShader* shader;
		VkPipeline pipeline;
		VkPipelineLayout pipelineLayout;
	};
}
