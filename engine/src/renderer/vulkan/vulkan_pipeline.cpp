#include "renderer/vulkan/vulkan_pipeline.h"

#include "renderer/vulkan/vulkan_utils.h"
#include "renderer/vulkan/vulkan_device.h"
#include "renderer/mesh.h"
#include "renderer/shader_cache.h"

#include "util/filesystem.h"
#include "util/log.h"

namespace MongooseVK
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

    VkPipelineColorBlendAttachmentState VulkanPipelineBuilder::ADDITIVE_BLENDING = {
        VK_TRUE,
        VK_BLEND_FACTOR_SRC_ALPHA,
        VK_BLEND_FACTOR_ONE,
        VK_BLEND_OP_ADD,
        VK_BLEND_FACTOR_ONE,
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_OP_ADD,
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendAttachmentState VulkanPipelineBuilder::ALPHA_BLENDING = {
        VK_TRUE,
        VK_BLEND_FACTOR_SRC_ALPHA,
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        VK_BLEND_OP_ADD,
        VK_BLEND_FACTOR_ONE,
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_OP_ADD,
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VulkanPipelineBuilder::VulkanPipelineBuilder() { clear(); }

    PipelineHandle VulkanPipelineBuilder::Build(VulkanDevice* vulkanDevice)
    {
        std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageCreateInfos;

        const auto vert_shader_code = ShaderCache::shaderCache.at(vertexShaderPath);
        const auto frag_shader_code = ShaderCache::shaderCache.at(fragmentShaderPath);

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
        for (auto& handle: descriptorSetLayouts)
        {
            auto descriptorSetLayout = vulkanDevice->GetDescriptorSetLayout(handle);
            vkDescriptorSetLayouts.push_back(descriptorSetLayout->descriptorSetLayout);
        }

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = vkDescriptorSetLayouts.size();
        pipelineLayoutInfo.pSetLayouts = vkDescriptorSetLayouts.data();

        if (!pushConstantRanges.empty())
        {
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
        pipelineInfo.renderPass = renderpass;
        pipelineInfo.subpass = 0;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pNext = &renderInfo;

        VkPipeline pipeline{};
        VK_CHECK_MSG(
            vkCreateGraphicsPipelines(vulkanDevice->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline),
            "Failed to create graphics pipeline.");

        PipelineHandle pipelineHandle = vulkanDevice->CreatePipeline();
        VulkanPipeline* vkPipeline = vulkanDevice->GetPipeline(pipelineHandle);

        vkPipeline->pipeline = pipeline;
        vkPipeline->pipelineLayout = pipelineLayout;

        vkPipeline->vertexShaderModule = vertexShaderModule;
        vkPipeline->fragmentShaderModule = fragmentShaderModule;

        for (size_t i = 0; i < descriptorSetLayouts.size(); i++)
            vkPipeline->descriptorSetLayouts[i] = descriptorSetLayouts[i];

        return pipelineHandle;
    }

    PipelineHandle VulkanPipelineBuilder::Build(VulkanDevice* vulkanDevice, PipelineCreate& config)
    {
        // SPR-V Shader source
        vertexShaderPath = config.vertexShaderPath;
        fragmentShaderPath = config.fragmentShaderPath;

        // Various flags
        polygonMode = Utils::ConvertPolygonMode(config.polygonMode);

        cullMode = Utils::ConvertCullMode(config.cullMode);
        frontFace = Utils::ConvertFrontFace(config.frontFace);

        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f;
        multisampling.pSampleMask = nullptr;
        multisampling.alphaToCoverageEnable = VK_FALSE;
        multisampling.alphaToOneEnable = VK_FALSE;

        renderpass = config.renderPass;

        disableBlending = config.disableBlending;

        // Depth testing
        if (config.enableDepthTest)
        {
            depthStencil.depthTestEnable = VK_TRUE;
            depthStencil.depthWriteEnable = config.depthWriteEnable ? VK_TRUE : VK_FALSE;
            depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
            depthStencil.depthBoundsTestEnable = VK_FALSE;
            depthStencil.minDepthBounds = 0.0f;
            depthStencil.maxDepthBounds = 1.0f;
            depthStencil.stencilTestEnable = VK_FALSE;
            depthStencil.back.failOp = VK_STENCIL_OP_KEEP;
            depthStencil.back.passOp = VK_STENCIL_OP_KEEP;
            depthStencil.back.compareOp = VK_COMPARE_OP_ALWAYS;
            depthStencil.front = depthStencil.back;

            renderInfo.depthAttachmentFormat = VulkanUtils::ConvertImageFormat(config.depthAttachment);
        } else
        {
            depthStencil.depthTestEnable = VK_FALSE;
            depthStencil.depthWriteEnable = VK_FALSE;
            depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
            depthStencil.depthBoundsTestEnable = VK_FALSE;
            depthStencil.minDepthBounds = 0.0f;
            depthStencil.maxDepthBounds = 1.0f;
            depthStencil.stencilTestEnable = VK_FALSE;
            depthStencil.front = {};
            depthStencil.back = {};
        }

        // Descriptor Set Layout

        for (const auto& layoutHandle: config.descriptorSetLayouts)
        {
            descriptorSetLayouts.push_back(layoutHandle);
        }

        // Color attachments
        for (const auto& colorAttachment: config.colorAttachments)
        {
            colorAttachmentFormats.push_back(VulkanUtils::ConvertImageFormat(colorAttachment));
            colorBlendAttachments.push_back(ALPHA_BLENDING);
        }

        // Optional push constant
        if (config.pushConstantData.size > 0)
        {
            VkPushConstantRange pushConstantRange{};
            pushConstantRange.stageFlags = config.pushConstantData.shaderStageBits;
            pushConstantRange.offset = config.pushConstantData.offset;
            pushConstantRange.size = config.pushConstantData.size;

            pushConstantRanges.push_back(pushConstantRange);
        }

        return Build(vulkanDevice);
    }

    void VulkanPipelineBuilder::clear()
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
