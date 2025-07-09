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

namespace MongooseVK
{
    class VulkanDevice;

    struct SimplePushConstantData {
        glm::mat4 transform{1.f};
        glm::mat4 modelMatrix{1.f};
    };

    struct TransformPushConstantData {
        glm::mat4 projection{1.f};
        glm::mat4 view{1.f};
    };

    struct PrefilterData {
        glm::mat4 projection{1.f};
        glm::mat4 view{1.f};
        float roughness = 1.0f;
        uint32_t resolution = 512;
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

        std::vector<Ref<VulkanDescriptorSetLayout>> descriptorSetLayouts{};
    };


    enum class PipelinePolygonMode {
        Unknown = 0,
        Point,
        Line,
        Fill,
    };

    enum class PipelineCullMode {
        Unknown = 0,
        Front,
        Back,
        Front_and_Back,
    };

    enum class PipelineFrontFace {
        Counter_clockwise = 0,
        Clockwise,
    };

    struct PipelinePushConstantData {
        VkShaderStageFlags shaderStageBits;
        uint32_t offset = 0;
        uint32_t size = 0;
    };

    struct PipelineConfig {
        std::string vertexShaderPath;
        std::string fragmentShaderPath;

        std::vector<Ref<VulkanDescriptorSetLayout>> descriptorSetLayouts{};
        std::vector<ImageFormat> colorAttachments;
        ImageFormat depthAttachment = ImageFormat::DEPTH24_STENCIL8;

        PipelinePolygonMode polygonMode = PipelinePolygonMode::Fill;
        PipelineFrontFace frontFace = PipelineFrontFace::Counter_clockwise;
        PipelineCullMode cullMode = PipelineCullMode::Back;

        Ref<VulkanRenderPass> renderPass = VK_NULL_HANDLE;

        PipelinePushConstantData pushConstantData{};

        bool enableDepthTest = true;
        bool disableBlending = true;
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
            Builder& AddDescriptorSetLayout(Ref<VulkanDescriptorSetLayout> _descriptorSetLayout);
            Builder& DisableBlending();
            Builder& AddColorAttachment(ImageFormat format, VkPipelineColorBlendAttachmentState blendState = ALPHA_BLENDING);
            Builder& SetDepthFormat(ImageFormat format);
            Builder& EnableDepthTest();
            Builder& DisableDepthTest();
            Builder& AddPushConstant(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size);
            Builder& SetRenderpass(Ref<VulkanRenderPass> _renderpass);

            Ref<VulkanPipeline> Build(VulkanDevice* vulkanDevice);
            Ref<VulkanPipeline> Build(VulkanDevice* vulkanDevice, PipelineConfig& config);

        private:
            void clear();

        public:
            static VkPipelineColorBlendAttachmentState ADDITIVE_BLENDING;
            static VkPipelineColorBlendAttachmentState ALPHA_BLENDING;

        private:
            std::string vertexShaderPath;
            std::string fragmentShaderPath;
            VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
            VkPipelineRasterizationStateCreateInfo rasterizer{};

            std::vector<VkFormat> colorAttachmentFormats;
            std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments{};

            VkPipelineMultisampleStateCreateInfo multisampling{};
            VkPipelineDepthStencilStateCreateInfo depthStencil{};
            VkPipelineRenderingCreateInfo renderInfo{};

            std::vector<VkPushConstantRange> pushConstantRanges{};

            VkPolygonMode polygonMode;
            VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            VkCullModeFlags cullMode;
            VkFrontFace frontFace;

            std::vector<Ref<VulkanDescriptorSetLayout>> descriptorSetLayouts;

            bool disableBlending = false;

            Ref<VulkanRenderPass> renderpass;
        };

    public:
        explicit VulkanPipeline(VulkanDevice* _vulkanDevice, PipelineParams& _params): vulkanDevice(_vulkanDevice), params(_params) {}
        ~VulkanPipeline();

        VkPipeline GetPipeline() const { return params.pipeline; }
        VkPipelineLayout GetPipelineLayout() const { return params.pipelineLayout; }
        std::vector<Ref<VulkanDescriptorSetLayout>> GetDescriptorSetLayouts() const { return params.descriptorSetLayouts; }

    private:
        VulkanDevice* vulkanDevice;
        PipelineParams params;
    };
}
