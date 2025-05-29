#include "shader_cache.h"

#include "util/filesystem.h"
#include "util/log.h"
#include "vulkan/vulkan_shader_compiler.h"

namespace Raytracing
{
    namespace Utils
    {
        shaderc_shader_kind GetShaderKindFromExtension(const std::filesystem::path& extension)
        {
            if (extension == ".vert") return shaderc_glsl_vertex_shader;
            if (extension == ".frag") return shaderc_glsl_fragment_shader;
            if (extension == ".geom") return shaderc_glsl_geometry_shader;
            if (extension == ".comp") return shaderc_glsl_compute_shader;

            ASSERT(false, "Unknown shader extension");
            return shaderc_glsl_vertex_shader;
        }
    }


    DescriptorSetLayouts ShaderCache::descriptorSetLayouts;
    DescriptorSets ShaderCache::descriptorSets;
    Renderpass ShaderCache::renderpasses;
    Pipelines ShaderCache::pipelines;
    std::unordered_map<std::string, std::vector<uint32_t>> ShaderCache::shaderCache;

    void ShaderCache::Load()
    {
        LoadShaders();
        LoadDescriptorLayouts();
        LoadRenderpasses();
        LoadPipelines();
    }

    void ShaderCache::LoadShaders()
    {
        VulkanShaderCompiler compiler;
        const auto glslFiles = FileSystem::GetFilesFromDirectory(SHADER_PATH, true);

        for (const auto& file: glslFiles)
        {
            CompilationInfo compilationInfo;
            compilationInfo.fileName = SHADER_PATH + file.filename().string();
            compilationInfo.kind = Utils::GetShaderKindFromExtension(file.extension());

            const std::vector<uint32_t> sprv_source = compiler.CompileFile(compilationInfo);
            shaderCache[file.filename().string()] = sprv_source;
        }
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

        // SSAO
        descriptorSetLayouts.postProcessingDescriptorSetLayout = VulkanDescriptorSetLayout::Builder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .Build();

        // Box Blur
        descriptorSetLayouts.ppBoxBlurDescriptorSetLayout = VulkanDescriptorSetLayout::Builder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
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
