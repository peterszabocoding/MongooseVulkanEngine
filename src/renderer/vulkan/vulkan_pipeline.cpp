#include "vulkan_pipeline.h"

#include "vulkan_utils.h"
#include "renderer/mesh.h"
#include "util/filesystem.h"

#include "vulkan_device.h"
#include "util/log.h"

namespace Raytracing
{
    namespace Utils
    {
        static VkPolygonMode ConvertPolygonMode(const PipelinePolygonMode polygonMode)
        {
            switch (polygonMode)
            {
                case PipelinePolygonMode::Point:
                    return VK_POLYGON_MODE_POINT;

                case PipelinePolygonMode::Line:
                    return VK_POLYGON_MODE_LINE;

                case PipelinePolygonMode::Fill:
                    return VK_POLYGON_MODE_FILL;

                default: ASSERT(false, "Unknown polygon mode");
            }
            return VK_POLYGON_MODE_MAX_ENUM;
        }

        static VkCullModeFlags ConvertCullMode(const PipelineCullMode cullMode)
        {
            switch (cullMode)
            {
                case PipelineCullMode::Front:
                    return VK_CULL_MODE_FRONT_BIT;
                case PipelineCullMode::Back:
                    return VK_CULL_MODE_BACK_BIT;
                case PipelineCullMode::Front_and_Back:
                    return VK_CULL_MODE_FRONT_AND_BACK;

                default:
                    return VK_CULL_MODE_NONE;
            }
        }

        static VkFrontFace ConvertFrontFace(const PipelineFrontFace frontFace)
        {
            switch (frontFace)
            {
                case PipelineFrontFace::Counter_clockwise:
                    return VK_FRONT_FACE_COUNTER_CLOCKWISE;

                case PipelineFrontFace::Clockwise:
                    return VK_FRONT_FACE_CLOCKWISE;

                default:
                    return VK_FRONT_FACE_COUNTER_CLOCKWISE;
            }
        }
    }

    VulkanPipeline::~VulkanPipeline()
    {
        vkDestroyShaderModule(vulkanDevice->GetDevice(), params.vertexShaderModule, nullptr);
        vkDestroyShaderModule(vulkanDevice->GetDevice(), params.fragmentShaderModule, nullptr);
        vkDestroyPipeline(vulkanDevice->GetDevice(), params.pipeline, nullptr);
        vkDestroyPipelineLayout(vulkanDevice->GetDevice(), params.pipelineLayout, nullptr);
    }

    VulkanPipeline::Builder::Builder() { clear(); }

    Ref<VulkanPipeline> VulkanPipeline::Builder::Build(VulkanDevice* vulkanDevice)
    {
        std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageCreateInfos;

        const auto vert_shader_code = FileSystem::ReadFile(vertexShaderPath);
        const auto frag_shader_code = FileSystem::ReadFile(fragmentShaderPath);

        VkShaderModule vertexShaderModule = VulkanUtils::CreateShaderModule(vulkanDevice->GetDevice(), vert_shader_code);
        VkShaderModule fragmentShaderModule = VulkanUtils::CreateShaderModule(vulkanDevice->GetDevice(), frag_shader_code);

        VkPipelineShaderStageCreateInfo vert_shader_stage_create_info{};
        vert_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vert_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vert_shader_stage_create_info.module = vertexShaderModule;
        vert_shader_stage_create_info.pName = "main";
        pipelineShaderStageCreateInfos.push_back(vert_shader_stage_create_info);

        VkPipelineShaderStageCreateInfo frag_shader_stage_create_info{};
        frag_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        frag_shader_stage_create_info.module = fragmentShaderModule;
        frag_shader_stage_create_info.pName = "main";
        pipelineShaderStageCreateInfos.push_back(frag_shader_stage_create_info);

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

        if (disableBlending)
        {
            for (auto& attachment: colorBlendAttachments)
            {
                // default write mask
                attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
                                            | VK_COLOR_COMPONENT_G_BIT
                                            | VK_COLOR_COMPONENT_B_BIT
                                            | VK_COLOR_COMPONENT_A_BIT;
                // no blending
                attachment.blendEnable = VK_FALSE;
            }
        }

        VkPipelineColorBlendStateCreateInfo color_blending{};
        color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blending.attachmentCount = colorBlendAttachments.size();
        color_blending.pAttachments = colorBlendAttachments.data();

        std::vector dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();


        std::vector<VkDescriptorSetLayout> vkDescriptorSetLayouts{};
        for (auto& descriptorSetLayout: descriptorSetLayouts)
            vkDescriptorSetLayouts.push_back(descriptorSetLayout->GetDescriptorSetLayout());

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = vkDescriptorSetLayouts.size();
        pipelineLayoutInfo.pSetLayouts = vkDescriptorSetLayouts.data();


        if (!pushConstantRanges.empty())
        {
            if (pushConstantRanges[0].stageFlags & VK_SHADER_STAGE_VERTEX_BIT)
                LOG_INFO("Push constant stage: Vertex");

            if (pushConstantRanges[0].stageFlags & VK_SHADER_STAGE_FRAGMENT_BIT)
                LOG_INFO("Push constant stage: Fragment");

            pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
            pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();
        }

        VkPipelineLayout pipelineLayout{};
        VK_CHECK_MSG(
            vkCreatePipelineLayout(vulkanDevice->GetDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout),
            "Failed to create pipeline layout.");

        renderInfo.colorAttachmentCount = colorAttachmentFormats.size();
        renderInfo.pColorAttachmentFormats = colorAttachmentFormats.data();

        inputAssembly.topology = topology;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        rasterizer.polygonMode = polygonMode;
        rasterizer.cullMode = cullMode;
        rasterizer.frontFace = frontFace;
        rasterizer.lineWidth = 1.0f;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = pipelineShaderStageCreateInfos.data();
        pipelineInfo.pVertexInputState = &vertex_input_info;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewport_state;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &color_blending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderpass->Get();
        pipelineInfo.subpass = 0;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pNext = &renderInfo;

        VkPipeline pipeline{};
        VK_CHECK_MSG(
            vkCreateGraphicsPipelines(vulkanDevice->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline),
            "Failed to create graphics pipeline.");

        PipelineParams params{};
        params.pipeline = pipeline;
        params.pipelineLayout = pipelineLayout;
        params.fragmentShaderModule = fragmentShaderModule;
        params.vertexShaderModule = vertexShaderModule;

        params.vertexShaderPath = vertexShaderPath;
        params.fragmentShaderPath = fragmentShaderPath;

        params.descriptorSetLayouts = descriptorSetLayouts;

        return CreateRef<VulkanPipeline>(vulkanDevice, params);
    }

    Ref<VulkanPipeline> VulkanPipeline::Builder::Build(VulkanDevice* vulkanDevice, PipelineConfig& config)
    {
        // SPR-V Shader source
        SetShaders(config.vertexShaderPath, config.fragmentShaderPath);

        // Various flags
        SetPolygonMode(Utils::ConvertPolygonMode(config.polygonMode));
        SetCullMode(Utils::ConvertCullMode(config.cullMode), Utils::ConvertFrontFace(config.frontFace));
        SetMultisampling();
        SetRenderpass(config.renderPass);
        if (config.disableBlending) DisableBlending();

        // Depth testing
        if (config.enableDepthTest)
        {
            EnableDepthTest();
            SetDepthFormat(config.depthAttachment);
        } else
        {
            DisableDepthTest();
        }

        // Descriptor Set Layout

        for (const auto& layout : config.descriptorSetLayouts)
            AddDescriptorSetLayout(layout);

        // Color attachments
        for (const auto& colorAttachment: config.colorAttachments)
            AddColorAttachment(colorAttachment);


        // Optional push constant
        if (config.pushConstantData.size > 0)
        {
            AddPushConstant(config.pushConstantData.shaderStageBits,
                            config.pushConstantData.offset,
                            config.pushConstantData.size);
        }

        return Build(vulkanDevice);
    }

    VulkanPipeline::Builder& VulkanPipeline::Builder::SetShaders(const std::string& _vertexShaderPath,
                                                                 const std::string& _fragmentShaderPath)
    {
        vertexShaderPath = _vertexShaderPath;
        fragmentShaderPath = _fragmentShaderPath;
        return *this;
    }

    VulkanPipeline::Builder& VulkanPipeline::Builder::SetInputTopology(const VkPrimitiveTopology _topology)
    {
        topology = _topology;
        return *this;
    }

    VulkanPipeline::Builder& VulkanPipeline::Builder::SetPolygonMode(const VkPolygonMode _polygonMode)
    {
        polygonMode = _polygonMode;
        return *this;
    }

    VulkanPipeline::Builder& VulkanPipeline::Builder::SetCullMode(const VkCullModeFlags _cullMode, const VkFrontFace _frontFace)
    {
        cullMode = _cullMode;
        frontFace = _frontFace;
        return *this;
    }

    VulkanPipeline::Builder& VulkanPipeline::Builder::SetMultisampling(const VkSampleCountFlagBits sampleCountFlagBits)
    {
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = sampleCountFlagBits;
        multisampling.minSampleShading = 1.0f;
        multisampling.pSampleMask = nullptr;
        multisampling.alphaToCoverageEnable = VK_FALSE;
        multisampling.alphaToOneEnable = VK_FALSE;
        return *this;
    }

    VulkanPipeline::Builder& VulkanPipeline::Builder::AddDescriptorSetLayout(Ref<VulkanDescriptorSetLayout> _descriptorSetLayout)
    {
        descriptorSetLayouts.push_back(_descriptorSetLayout);
        return *this;
    }

    VulkanPipeline::Builder& VulkanPipeline::Builder::DisableBlending()
    {
        disableBlending = true;
        return *this;
    }

    VulkanPipeline::Builder& VulkanPipeline::Builder::AddColorAttachment(const ImageFormat format)
    {
        colorAttachmentFormats.push_back(VulkanUtils::ConvertImageFormat(format));
        colorBlendAttachments.push_back({});
        return *this;
    }

    VulkanPipeline::Builder& VulkanPipeline::Builder::SetDepthFormat(const ImageFormat format)
    {
        renderInfo.depthAttachmentFormat = VulkanUtils::ConvertImageFormat(format);
        return *this;
    }

    VulkanPipeline::Builder& VulkanPipeline::Builder::AddPushConstant(const VkShaderStageFlags stageFlags, const uint32_t offset,
                                                                      const uint32_t size)
    {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = stageFlags;
        pushConstantRange.offset = offset;
        pushConstantRange.size = size;

        pushConstantRanges.push_back(pushConstantRange);
        return *this;
    }

    VulkanPipeline::Builder& VulkanPipeline::Builder::SetRenderpass(Ref<VulkanRenderPass> _renderpass)
    {
        renderpass = _renderpass;
        return *this;
    }

    VulkanPipeline::Builder& VulkanPipeline::Builder::EnableDepthTest()
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

    VulkanPipeline::Builder& VulkanPipeline::Builder::DisableDepthTest()
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

    void VulkanPipeline::Builder::clear()
    {
        inputAssembly = {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

        rasterizer = {};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

        colorBlendAttachments.clear();
        colorAttachmentFormats.clear();

        multisampling = {};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

        depthStencil = {};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

        renderInfo = {};
        renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    }
}
