#pragma once
#include <string>
#include <vulkan/vulkan_core.h>
#include "vulkan_shader.h"

namespace Raytracing
{
    class VulkanDevice;

    class VulkanPipeline {
    public:
        explicit VulkanPipeline(VulkanDevice* vulkanDevice, Ref<VulkanShader> shader, VkPipeline pipeline, VkPipelineLayout pipelineLayout);
        ~VulkanPipeline();

        VkPipeline GetPipeline() const { return pipeline; }
        VkPipelineLayout GetPipelineLayout() const { return pipelineLayout; }
        Ref<VulkanShader> GetShader() const { return shader; }

    private:
        VulkanDevice* vulkanDevice;
        Ref<VulkanShader> shader;
        VkPipeline pipeline{};
        VkPipelineLayout pipelineLayout{};
    };

    class PipelineBuilder {
    public:
        PipelineBuilder();
        ~PipelineBuilder() = default;
        Ref<VulkanPipeline> Build(VulkanDevice* vulkanDevice) const;

    public:
        PipelineBuilder& SetShaders(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
        PipelineBuilder& SetInputTopology(VkPrimitiveTopology topology);
        PipelineBuilder& SetPolygonMode(VkPolygonMode mode);
        PipelineBuilder& SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
        PipelineBuilder& SetMultisampling(VkSampleCountFlagBits sampleCountFlagBits = VK_SAMPLE_COUNT_1_BIT);
        PipelineBuilder& DisableBlending();
        PipelineBuilder& SetColorAttachmentFormat(VkFormat format);
        PipelineBuilder& SetDepthFormat(VkFormat format);
        PipelineBuilder& EnableDepthTest();
        PipelineBuilder& DisableDepthTest();
        PipelineBuilder& AddPushConstant(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size);

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

        std::vector<VkPushConstantRange> pushConstantRanges;

    private:
        void clear();
    };
}
