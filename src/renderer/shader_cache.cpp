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

        // world-space normals, position, depth
        descriptorSetLayouts.gBufferDescriptorSetLayout = VulkanDescriptorSetLayout::Builder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .AddBinding({1, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .AddBinding({2, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .Build();

        // samples, noise texture
        descriptorSetLayouts.ssaoDescriptorSetLayout = VulkanDescriptorSetLayout::Builder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::UniformBuffer, {ShaderStage::FragmentShader}})
                .AddBinding({1, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .Build();
    }

    void ShaderCache::LoadPipelines()
    {
        LOG_TRACE("Build pipelines");
    }

    void ShaderCache::LoadRenderpasses()
    {
        renderpasses.iblPreparePass = VulkanRenderPass::Builder(vulkanDevice)
                .AddColorAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, false)
                .Build();
    }
}
