//
// Created by peter on 2025. 05. 11..
//

#include "shader_cache.h"

#include "util/log.h"

namespace Raytracing
{
    DescriptorSetLayouts ShaderCache::descriptorSetLayouts;
    DescriptorSets ShaderCache::descriptorSets;
    Renderpass ShaderCache::renderpasses;
    Pipelines ShaderCache::pipelines;

    void ShaderCache::Load()
    {
        LoadDescriptorLayouts();
        LoadRenderpasses();
        LoadPipelines();
    }

    void ShaderCache::LoadDescriptorLayouts()
    {

        // transforms
        descriptorSetLayouts.transformDescriptorSetLayout = VulkanDescriptorSetLayout::Builder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::UniformBuffer, {ShaderStage::VertexShader, ShaderStage::FragmentShader}})
                .Build();

        // materialParams
        descriptorSetLayouts.materialDescriptorSetLayout = VulkanDescriptorSetLayout::Builder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::UniformBuffer, {ShaderStage::FragmentShader}})
                .AddBinding({1, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .AddBinding({2, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .AddBinding({3, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .Build();

        // skyboxSampler
        descriptorSetLayouts.cubemapDescriptorSetLayout = VulkanDescriptorSetLayout::Builder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .Build();

        // lights, shadowMap
        descriptorSetLayouts.lightsDescriptorSetLayout = VulkanDescriptorSetLayout::Builder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::UniformBuffer, {ShaderStage::VertexShader, ShaderStage::FragmentShader}})
                .AddBinding({1, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .Build();

        // imageSampler
        descriptorSetLayouts.presentDescriptorSetLayout = VulkanDescriptorSetLayout::Builder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .Build();

        // irradianceMap, prefilterMap, brdfLUT
        descriptorSetLayouts.pbrDescriptorSetLayout = VulkanDescriptorSetLayout::Builder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .AddBinding({1, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .AddBinding({2, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .Build();
    }

    void ShaderCache::LoadPipelines()
    {
        LOG_TRACE("Build pipelines");

        LOG_TRACE("Building skybox pipeline");
        PipelineConfig skyboxPipelineConfig; {
            skyboxPipelineConfig.vertexShaderPath = "shader/spv/skybox.vert.spv";
            skyboxPipelineConfig.fragmentShaderPath = "shader/spv/skybox.frag.spv";

            skyboxPipelineConfig.cullMode = PipelineCullMode::Front;
            skyboxPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            skyboxPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

            skyboxPipelineConfig.descriptorSetLayouts = {
                descriptorSetLayouts.cubemapDescriptorSetLayout,
                descriptorSetLayouts.transformDescriptorSetLayout,
            };

            skyboxPipelineConfig.colorAttachments = {
                ImageFormat::RGBA8_UNORM,
            };

            skyboxPipelineConfig.disableBlending = true;
            skyboxPipelineConfig.enableDepthTest = false;
            skyboxPipelineConfig.depthAttachment = ImageFormat::DEPTH24_STENCIL8;

            skyboxPipelineConfig.renderPass = renderpasses.geometryPass;
        }
        pipelines.skyBox = VulkanPipeline::Builder().Build(vulkanDevice, skyboxPipelineConfig);

        LOG_TRACE("Building geometry pipeline");
        PipelineConfig geometryPipelineConfig; {
            geometryPipelineConfig.vertexShaderPath = "shader/spv/gbuffer.vert.spv";
            geometryPipelineConfig.fragmentShaderPath = "shader/spv/gbuffer.frag.spv";

            geometryPipelineConfig.cullMode = PipelineCullMode::Back;
            geometryPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            geometryPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

            geometryPipelineConfig.descriptorSetLayouts = {
                descriptorSetLayouts.materialDescriptorSetLayout,
                descriptorSetLayouts.transformDescriptorSetLayout,
                descriptorSetLayouts.cubemapDescriptorSetLayout,
                descriptorSetLayouts.lightsDescriptorSetLayout,
                descriptorSetLayouts.pbrDescriptorSetLayout,
            };

            geometryPipelineConfig.colorAttachments = {
                ImageFormat::RGBA8_UNORM,
                ImageFormat::RGBA8_UNORM,
                ImageFormat::RGBA8_UNORM,
                ImageFormat::RGBA8_UNORM,
                ImageFormat::RGBA8_UNORM,
            };

            geometryPipelineConfig.disableBlending = true;
            geometryPipelineConfig.enableDepthTest = true;
            geometryPipelineConfig.depthAttachment = ImageFormat::DEPTH24_STENCIL8;

            geometryPipelineConfig.renderPass = renderpasses.geometryPass;

            geometryPipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            geometryPipelineConfig.pushConstantData.offset = 0;
            geometryPipelineConfig.pushConstantData.size = sizeof(SimplePushConstantData);
        }
        pipelines.geometry = VulkanPipeline::Builder().Build(vulkanDevice, geometryPipelineConfig);

        LOG_TRACE("Building directional shadow map pipeline");
        PipelineConfig dirShadowMapPipelineConfig; {
            dirShadowMapPipelineConfig.vertexShaderPath = "shader/spv/shadow_map.vert.spv";
            dirShadowMapPipelineConfig.fragmentShaderPath = "shader/spv/empty.frag.spv";

            dirShadowMapPipelineConfig.cullMode = PipelineCullMode::Front;
            dirShadowMapPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            dirShadowMapPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

            dirShadowMapPipelineConfig.descriptorSetLayouts = {
                descriptorSetLayouts.lightsDescriptorSetLayout,
            };

            dirShadowMapPipelineConfig.disableBlending = true;
            dirShadowMapPipelineConfig.enableDepthTest = true;
            dirShadowMapPipelineConfig.depthAttachment = ImageFormat::DEPTH32;

            dirShadowMapPipelineConfig.renderPass = renderpasses.shadowMapPass;

            dirShadowMapPipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            dirShadowMapPipelineConfig.pushConstantData.offset = 0;
            dirShadowMapPipelineConfig.pushConstantData.size = sizeof(SimplePushConstantData);
        }
        pipelines.directionalShadowMap = VulkanPipeline::Builder().Build(vulkanDevice, dirShadowMapPipelineConfig);

        LOG_TRACE("Building present pipeline");
        PipelineConfig presentPipelineConfig; {
            presentPipelineConfig.vertexShaderPath = "shader/spv/quad.vert.spv";
            presentPipelineConfig.fragmentShaderPath = "shader/spv/quad.frag.spv";

            presentPipelineConfig.cullMode = PipelineCullMode::Front;
            presentPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            presentPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

            presentPipelineConfig.descriptorSetLayouts = {
                descriptorSetLayouts.presentDescriptorSetLayout
            };

            presentPipelineConfig.colorAttachments = {
                ImageFormat::RGBA8_UNORM,
            };

            presentPipelineConfig.disableBlending = true;
            presentPipelineConfig.enableDepthTest = false;

            presentPipelineConfig.renderPass = renderpasses.presentPass;
        }
        pipelines.present = VulkanPipeline::Builder().Build(vulkanDevice, presentPipelineConfig);

        LOG_TRACE("Building IBL BRDF pipeline");
        PipelineConfig iblBrdfPipelineConfig; {
            iblBrdfPipelineConfig.vertexShaderPath = "shader/spv/brdf.vert.spv";
            iblBrdfPipelineConfig.fragmentShaderPath = "shader/spv/brdf.frag.spv";

            iblBrdfPipelineConfig.cullMode = PipelineCullMode::Front;
            iblBrdfPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            iblBrdfPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

            iblBrdfPipelineConfig.descriptorSetLayouts = {};

            iblBrdfPipelineConfig.colorAttachments = {
                ImageFormat::RGBA16_SFLOAT,
            };

            iblBrdfPipelineConfig.disableBlending = true;
            iblBrdfPipelineConfig.enableDepthTest = false;

            iblBrdfPipelineConfig.renderPass = renderpasses.iblPreparePass;
        }
        pipelines.ibl_brdf = VulkanPipeline::Builder().Build(vulkanDevice, iblBrdfPipelineConfig);

        LOG_TRACE("Building IBL Irradiance Map pipeline");
        PipelineConfig iblIrradianceMapPipelineConfig; {
            iblIrradianceMapPipelineConfig.vertexShaderPath = "shader/spv/cubemap.vert.spv";
            iblIrradianceMapPipelineConfig.fragmentShaderPath = "shader/spv/irradiance_convolution.frag.spv";

            iblIrradianceMapPipelineConfig.cullMode = PipelineCullMode::Back;
            iblIrradianceMapPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            iblIrradianceMapPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

            iblIrradianceMapPipelineConfig.descriptorSetLayouts = {
                descriptorSetLayouts.cubemapDescriptorSetLayout,
            };

            iblIrradianceMapPipelineConfig.colorAttachments = {
                ImageFormat::RGBA16_SFLOAT,
            };

            iblIrradianceMapPipelineConfig.disableBlending = true;
            iblIrradianceMapPipelineConfig.enableDepthTest = false;

            iblIrradianceMapPipelineConfig.renderPass = renderpasses.iblPreparePass;

            iblIrradianceMapPipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            iblIrradianceMapPipelineConfig.pushConstantData.offset = 0;
            iblIrradianceMapPipelineConfig.pushConstantData.size = sizeof(TransformPushConstantData);
        }
        pipelines.ibl_irradianceMap = VulkanPipeline::Builder().Build(vulkanDevice, iblIrradianceMapPipelineConfig);

        LOG_TRACE("Building IBL Prefilter pipeline");
        PipelineConfig iblPrefilterPipelineConfig; {
            iblPrefilterPipelineConfig.vertexShaderPath = "shader/spv/cubemap.vert.spv";
            iblPrefilterPipelineConfig.fragmentShaderPath = "shader/spv/prefilter.frag.spv";

            iblPrefilterPipelineConfig.cullMode = PipelineCullMode::Back;
            iblPrefilterPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            iblPrefilterPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

            iblPrefilterPipelineConfig.descriptorSetLayouts = {
                descriptorSetLayouts.cubemapDescriptorSetLayout,
            };

            iblPrefilterPipelineConfig.colorAttachments = {
                ImageFormat::RGBA16_SFLOAT,
            };

            iblPrefilterPipelineConfig.disableBlending = true;
            iblPrefilterPipelineConfig.enableDepthTest = false;

            iblPrefilterPipelineConfig.renderPass = renderpasses.iblPreparePass;
        }
        pipelines.ibl_prefilter = VulkanPipeline::Builder().Build(vulkanDevice, iblPrefilterPipelineConfig);
    }

    void ShaderCache::LoadRenderpasses()
    {
        LOG_TRACE("Vulkan: create renderpass");
        renderpasses.skyboxPass = VulkanRenderPass::Builder(vulkanDevice)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM, false)
                .AddDepthAttachment()
                .Build();

        renderpasses.geometryPass = VulkanRenderPass::Builder(vulkanDevice)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM, false)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM, false)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM, false)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM, false)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM, false)
                .AddDepthAttachment()
                .Build();

        renderpasses.shadowMapPass = VulkanRenderPass::Builder(vulkanDevice)
                .AddDepthAttachment(VK_FORMAT_D32_SFLOAT)
                .Build();

        renderpasses.presentPass = VulkanRenderPass::Builder(vulkanDevice)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM, true)
                .Build();

        renderpasses.iblPreparePass = VulkanRenderPass::Builder(vulkanDevice)
                .AddColorAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, false)
                .Build();
    }
}
