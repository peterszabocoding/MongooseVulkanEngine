#include "renderer/shader_cache.h"

#include <renderer/vulkan/vulkan_device.h>

#include "util/filesystem.h"
#include "util/log.h"
#include "renderer/vulkan/vulkan_shader_compiler.h"

namespace MongooseVK
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
    std::unordered_map<std::string, std::vector<uint32_t>> ShaderCache::shaderCache;

    void ShaderCache::Load()
    {
        LoadShaders();
        LoadDescriptorLayouts();
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
        descriptorSetLayouts.cameraDescriptorSetLayout = VulkanDescriptorSetLayoutBuilder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::UniformBuffer, {ShaderStage::VertexShader, ShaderStage::FragmentShader}})
                .Build();

        // materialParams
        descriptorSetLayouts.materialDescriptorSetLayout = VulkanDescriptorSetLayoutBuilder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::UniformBuffer, {ShaderStage::FragmentShader}})
                .Build();

        // skyboxSampler
        descriptorSetLayouts.cubemapDescriptorSetLayout = VulkanDescriptorSetLayoutBuilder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .Build();

        // lights
        descriptorSetLayouts.lightsDescriptorSetLayout = VulkanDescriptorSetLayoutBuilder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::UniformBuffer, {ShaderStage::VertexShader, ShaderStage::FragmentShader}})
                .Build();
        // directional shadow map
        descriptorSetLayouts.directionalShadowMapDescriptorSetLayout = VulkanDescriptorSetLayoutBuilder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .Build();

        // imageSampler
        descriptorSetLayouts.presentDescriptorSetLayout = VulkanDescriptorSetLayoutBuilder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .Build();

        // irradianceMap
        descriptorSetLayouts.irradianceDescriptorSetLayout = VulkanDescriptorSetLayoutBuilder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .Build();

        // prefilterMap,
        descriptorSetLayouts.reflectionDescriptorSetLayout = VulkanDescriptorSetLayoutBuilder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .Build();

        // brdfLUT
        descriptorSetLayouts.brdfLutDescriptorSetLayout = VulkanDescriptorSetLayoutBuilder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .Build();

        // world-space normals
        descriptorSetLayouts.viewspaceNormalDescriptorSetLayout = VulkanDescriptorSetLayoutBuilder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .Build();

        // position
        descriptorSetLayouts.viewspacePositionDescriptorSetLayout = VulkanDescriptorSetLayoutBuilder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .Build();
        // depth
        descriptorSetLayouts.depthMapDescriptorSetLayout = VulkanDescriptorSetLayoutBuilder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .Build();

        // samples, noise texture
        descriptorSetLayouts.ssaoDescriptorSetLayout = VulkanDescriptorSetLayoutBuilder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::UniformBuffer, {ShaderStage::FragmentShader}})
                .AddBinding({1, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .Build();

        // SSAO
        descriptorSetLayouts.postProcessingDescriptorSetLayout = VulkanDescriptorSetLayoutBuilder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .Build();

        // Box Blur
        descriptorSetLayouts.ppBoxBlurDescriptorSetLayout = VulkanDescriptorSetLayoutBuilder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .Build();

        // Tone Mapping
        descriptorSetLayouts.toneMappingDescriptorSetLayout = VulkanDescriptorSetLayoutBuilder(vulkanDevice)
                .AddBinding({0, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .Build();
    }
}
