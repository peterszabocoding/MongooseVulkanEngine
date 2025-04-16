#include "vulkan_pipeline.h"

#include "vulkan_utils.h"
#include "renderer/mesh.h"
#include "util/filesystem.h"

#include "vulkan_device.h"

namespace Raytracing
{
    VulkanPipeline::VulkanPipeline(VulkanDevice* vulkanDevice, Ref<VulkanShader> shader, VkPipeline pipeline,
                                   VkPipelineLayout pipelineLayout)
        : vulkanDevice(vulkanDevice), shader(shader), pipeline(pipeline), pipelineLayout(pipelineLayout)
    {}

    VulkanPipeline::~VulkanPipeline()
    {
        vkDestroyPipeline(vulkanDevice->GetDevice(), pipeline, nullptr);
        vkDestroyPipelineLayout(vulkanDevice->GetDevice(), pipelineLayout, nullptr);
    }

    PipelineBuilder::PipelineBuilder() { clear(); }

    Ref<VulkanPipeline> PipelineBuilder::Build(VulkanDevice* vulkanDevice) const
    {
        Ref<VulkanShader> shader = CreateRef<VulkanShader>(vulkanDevice, vertexShaderPath, fragmentShaderPath);

        auto binding_description = VulkanVertex::GetBindingDescription();
        auto attribute_descriptions = VulkanVertex::GetAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertex_input_info{};
        vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_info.vertexBindingDescriptionCount = 1;
        vertex_input_info.pVertexBindingDescriptions = &binding_description;
        vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
        vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

        VkPipelineViewportStateCreateInfo viewport_state{};
        viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.viewportCount = 1;
        viewport_state.scissorCount = 1;

        VkPipelineColorBlendStateCreateInfo color_blending{};
        color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blending.logicOpEnable = VK_FALSE;
        color_blending.logicOp = VK_LOGIC_OP_COPY;
        color_blending.attachmentCount = 1;
        color_blending.pAttachments = &colorBlendAttachment;

        std::vector dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &shader->GetDescriptorSetLayout();
        pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
        pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

        VkPipelineLayout pipelineLayout{};
        VK_CHECK_MSG(
            vkCreatePipelineLayout(vulkanDevice->GetDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout),
            "Failed to create pipeline layout.");

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shader->GetPipelineShaderStageCreateInfos().data();
        pipelineInfo.pVertexInputState = &vertex_input_info;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewport_state;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &color_blending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = vulkanDevice->GetRenderPass();
        pipelineInfo.subpass = 0;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pNext = &renderInfo;

        VkPipeline pipeline{};
        VK_CHECK_MSG(
            vkCreateGraphicsPipelines(vulkanDevice->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline),
            "Failed to create graphics pipeline.");

        return CreateRef<VulkanPipeline>(vulkanDevice, shader, pipeline, pipelineLayout);
    }

    PipelineBuilder& PipelineBuilder::SetShaders(const std::string& vertexShaderPath,
                                     const std::string& fragmentShaderPath)
    {
        this->vertexShaderPath = vertexShaderPath;
        this->fragmentShaderPath = fragmentShaderPath;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::SetInputTopology(VkPrimitiveTopology topology)
    {
        inputAssembly.topology = topology;
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::SetPolygonMode(VkPolygonMode mode)
    {
        rasterizer.polygonMode = mode;
        rasterizer.lineWidth = 1.0f;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace)
    {
        rasterizer.cullMode = cullMode;
        rasterizer.frontFace = frontFace;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::SetMultisampling(VkSampleCountFlagBits sampleCountFlagBits)
    {
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = sampleCountFlagBits;
        multisampling.minSampleShading = 1.0f;
        multisampling.pSampleMask = nullptr;
        multisampling.alphaToCoverageEnable = VK_FALSE;
        multisampling.alphaToOneEnable = VK_FALSE;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::DisableBlending()
    {
        // default write mask
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                                              VK_COLOR_COMPONENT_A_BIT;
        // no blending
        colorBlendAttachment.blendEnable = VK_FALSE;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::SetColorAttachmentFormat(VkFormat format)
    {
        colorAttachmentformat = format;
        renderInfo.colorAttachmentCount = 1;
        renderInfo.pColorAttachmentFormats = &colorAttachmentformat;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::SetDepthFormat(VkFormat format)
    {
        renderInfo.depthAttachmentFormat = format;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::DisableDepthTest()
    {
        depthStencil.depthTestEnable = VK_FALSE;
        depthStencil.depthWriteEnable = VK_FALSE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {};
        depthStencil.back = {};
        depthStencil.minDepthBounds = 0.0f;
        depthStencil.maxDepthBounds = 1.0f;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::AddPushConstant(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size)
    {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = stageFlags;
        pushConstantRange.offset = offset;
        pushConstantRange.size = size;

        pushConstantRanges.push_back(pushConstantRange);
        return *this;
    }

    PipelineBuilder& PipelineBuilder::EnableDepthTest()
    {
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {}; // Optional
        return *this;
    }

    void PipelineBuilder::clear()
    {
        inputAssembly = {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

        rasterizer = {};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

        colorBlendAttachment = {};

        multisampling = {};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

        depthStencil = {};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

        renderInfo = {};
        renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    }
}
