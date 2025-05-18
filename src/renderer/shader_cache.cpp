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

        // irradianceMap
        descriptorSetLayouts.irradianceDescriptorSetLayout = VulkanDescriptorSetLayout::Builder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .Build();

        // prefilterMap, brdfLUT
        descriptorSetLayouts.reflectionDescriptorSetLayout = VulkanDescriptorSetLayout::Builder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .AddBinding({1, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .Build();
    }

    void ShaderCache::LoadPipelines()
    {
        LOG_TRACE("Build pipelines");
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
        renderpasses.iblPreparePass = VulkanRenderPass::Builder(vulkanDevice)
                .AddColorAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, false)
                .Build();
    }
}
