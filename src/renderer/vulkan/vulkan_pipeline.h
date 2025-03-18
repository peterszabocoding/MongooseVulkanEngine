#pragma once
#include <string>
#include <vulkan/vulkan_core.h>

namespace Raytracing
{
    class VulkanDevice;
    class VulkanShader;

    class VulkanPipeline {
    public:
        VulkanPipeline(VulkanDevice *vulkanDevice, VulkanShader *shader);
        ~VulkanPipeline();

        VkPipeline GetPipeline() const { return pipeline; }
        VkPipelineLayout GetPipelineLayout() const { return pipelineLayout; }

    private:
        void CreateGraphicsPipeline();

    private:
        VulkanDevice *vulkanDevice;
        VulkanShader *shader;

        VkPipeline pipeline{};
        VkPipelineLayout pipelineLayout{};
    };
}
