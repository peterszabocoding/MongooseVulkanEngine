#pragma once
#include <string>
#include <vulkan/vulkan_core.h>

#include "vulkan_descriptor_set_layout.h"
#include "util/core.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Raytracing
{
    class VulkanDevice;

    struct SimplePushConstantData {
        glm::mat4 transform{1.f};
        glm::mat4 normalMatrix{1.f};
    };

    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct PipelineParams {
        std::string vertexShaderPath;
        std::string fragmentShaderPath;

        VkShaderModule vertexShaderModule = VK_NULL_HANDLE;
        VkShaderModule fragmentShaderModule = VK_NULL_HANDLE;

        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;

        Ref<VulkanDescriptorSetLayout> descriptorSetLayout = VK_NULL_HANDLE;
    };

    class VulkanPipeline {
    public:
        class Builder {
        public:
            Builder();
            ~Builder() = default;

            Builder& SetShaders(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
            Builder& SetInputTopology(VkPrimitiveTopology topology);
            Builder& SetPolygonMode(VkPolygonMode mode);
            Builder& SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
            Builder& SetMultisampling(VkSampleCountFlagBits sampleCountFlagBits = VK_SAMPLE_COUNT_1_BIT);
            Builder& SetDescriptorSetLayout(Ref<VulkanDescriptorSetLayout> _descriptorSetLayout);
            Builder& DisableBlending();
            Builder& AddColorAttachment(VkFormat format);
            Builder& SetDepthFormat(VkFormat format);
            Builder& EnableDepthTest();
            Builder& DisableDepthTest();
            Builder& AddPushConstant(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size);

            Ref<VulkanPipeline> Build(VulkanDevice* vulkanDevice);

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

            Ref<VulkanDescriptorSetLayout> descriptorSetLayout;

            bool disableBlending = false;
        };

    public:
        explicit VulkanPipeline(VulkanDevice* _vulkanDevice, PipelineParams& _params): vulkanDevice(_vulkanDevice), params(_params) {}
        ~VulkanPipeline();

        VkPipeline GetPipeline() const { return params.pipeline; }
        VkPipelineLayout GetPipelineLayout() const { return params.pipelineLayout; }
        Ref<VulkanDescriptorSetLayout> GetDescriptorSetLayout() const { return params.descriptorSetLayout; }

    private:
        VulkanDevice* vulkanDevice;
        VkPipeline pipeline{};
        VkPipelineLayout pipelineLayout{};
        PipelineParams params;
    };
}
