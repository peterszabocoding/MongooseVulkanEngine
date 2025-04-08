#pragma once
#include <string>
#include <vulkan/vulkan_core.h>
#include "vulkan_shader.h"

namespace Raytracing
{
    class VulkanDevice;

    class VulkanPipeline {
    public:
        explicit VulkanPipeline(VulkanDevice* vulkanDevice, VulkanShader* shader, VkPipeline pipeline, VkPipelineLayout pipelineLayout);
        ~VulkanPipeline();

        VkPipeline GetPipeline() const { return pipeline; }
        VkPipelineLayout GetPipelineLayout() const { return pipelineLayout; }
        VulkanShader* GetShader() const { return shader; }

    private:
        VulkanDevice* vulkanDevice;
        VulkanShader* shader;
        VkPipeline pipeline{};
        VkPipelineLayout pipelineLayout{};
    };

    class PipelineBuilder {
    public:
        PipelineBuilder();
        ~PipelineBuilder() = default;
        VulkanPipeline* build(VulkanDevice* vulkanDevice) const;

    public:
        void SetShaders(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
        void SetInputTopology(VkPrimitiveTopology topology);
        void SetPolygonMode(VkPolygonMode mode);
        void SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
        void SetMultisampling(VkSampleCountFlagBits sampleCountFlagBits = VK_SAMPLE_COUNT_1_BIT);
        void DisableBlending();
        void SetColorAttachmentFormat(VkFormat format);
        void SetDepthFormat(VkFormat format);
        void EnableDepthTest();
        void DisableDepthTest();

    public:
        std::string vertexShaderPath;
        std::string fragmentShaderPath;
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        VkPipelineMultisampleStateCreateInfo multisampling{};
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        VkPipelineRenderingCreateInfo renderInfo{};
        VkFormat colorAttachmentformat;

    private:
        void clear();
    };
}
