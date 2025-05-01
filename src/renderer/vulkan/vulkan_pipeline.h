#pragma once
#include <string>
#include <vulkan/vulkan_core.h>

#include "vulkan_descriptor_set_layout.h"
#include "util/core.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "vulkan_renderpass.h"
#include "resource/resource.h"

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

    enum class PipelineBindingType {
        None = 0,
        UniformBuffer = 1,
        TextureSampler = 2,
    };

    enum class PipelineBindingStage {
        None = 0,
        VertexShader = 1,
        FragmentShader = 2,
        ComputeShader = 3,
        GeometryShader = 4,
    };

    enum class PipelinePolygonMode {
        None = 0,
        Point = 1,
        Line = 2,
        Fill = 3,
    };

    struct PipelineBinding {
        uint32_t binding = 0;
        PipelineBindingType type = PipelineBindingType::None;
        PipelineBindingStage stage = PipelineBindingStage::None;
    };

    struct PipelineConfig {
        std::string vertexShaderPath;
        std::string fragmentShaderPath;
        std::vector<PipelineBinding> bindings;
        PipelinePolygonMode polygonMode = PipelinePolygonMode::Fill;
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
            Builder& AddColorAttachment(ImageFormat format);
            Builder& SetDepthFormat(ImageFormat format);
            Builder& EnableDepthTest();
            Builder& DisableDepthTest();
            Builder& AddPushConstant(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size);
            Builder& SetRenderpass(Ref<VulkanRenderPass> _renderpass);

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

            Ref<VulkanRenderPass> renderpass;
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
