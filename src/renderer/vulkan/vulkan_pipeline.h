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
		explicit VulkanPipeline(VulkanDevice* vulkanDevice);
		~VulkanPipeline();

		VkPipeline GetPipeline() const { return pipeline; }
		VkPipelineLayout GetPipelineLayout() const { return pipelineLayout; }
		VulkanShader* GetShader() const { return shader; }
		void Load(
			const std::string& vertexShaderPath,
			const std::string& fragmentShaderPath);

	private:
		VulkanDevice* vulkanDevice;
		VulkanShader* shader;
		VkPipeline pipeline{};
		VkPipelineLayout pipelineLayout{};
	};
}
