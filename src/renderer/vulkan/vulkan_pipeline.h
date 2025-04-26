#pragma once
#include <string>
#include <vulkan/vulkan_core.h>
#include "vulkan_shader.h"

namespace Raytracing
{
    class VulkanDevice;

    class VulkanPipeline {
    public:
        class Builder {
        public:
            Builder();
            ~Builder() = default;
            Ref<VulkanPipeline> Build(VulkanDevice* vulkanDevice);

            Builder& SetShaders(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
            Builder& SetInputTopology(VkPrimitiveTopology topology);
            Builder& SetPolygonMode(VkPolygonMode mode);
            Builder& SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
            Builder& SetMultisampling(VkSampleCountFlagBits sampleCountFlagBits = VK_SAMPLE_COUNT_1_BIT);
            Builder& DisableBlending();
            Builder& AddColorAttachment(VkFormat format);
            Builder& SetDepthFormat(VkFormat format);
            Builder& EnableDepthTest();
            Builder& DisableDepthTest();
            Builder& AddPushConstant(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size);

        private:
            void clear();

        private:
        public:
            std::string vertexShaderPath;
            std::string fragmentShaderPath;
            VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
            VkPipelineRasterizationStateCreateInfo rasterizer{};

            std::vector<VkFormat> colorAttachmentFormats;
            std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments{};

            VkPipelineMultisampleStateCreateInfo multisampling{};
            VkPipelineDepthStencilStateCreateInfo depthStencil{};
            VkPipelineRenderingCreateInfo renderInfo{};

            std::vector<VkPushConstantRange> pushConstantRanges;

            VkPolygonMode polygonMode;
            VkPrimitiveTopology topology;
            VkCullModeFlags cullMode;
            VkFrontFace frontFace;

            bool disableBlending = false;
        };

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
}
