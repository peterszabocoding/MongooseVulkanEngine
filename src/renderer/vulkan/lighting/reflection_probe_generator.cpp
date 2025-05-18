//
// Created by peter on 2025. 05. 18..
//

#include "reflection_probe_generator.h"

#include "renderer/shader_cache.h"
#include "renderer/vulkan/vulkan_descriptor_writer.h"
#include "renderer/vulkan/vulkan_framebuffer.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "resource/resource_manager.h"

namespace Raytracing
{
    const glm::mat4 m_CaptureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    const glm::mat4 m_CaptureViews[6] = {
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
    };

    ReflectionProbeGenerator::ReflectionProbeGenerator(VulkanDevice* _device): device(_device)
    {
        renderPass = VulkanRenderPass::Builder(device)
                .AddColorAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, false)
                .Build();
        LoadPipeline();

        screenRect = CreateScope<VulkanMeshlet>(device, Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);
        cubeMesh = ResourceManager::LoadMesh(device, "resources/models/cube.obj");
    }

    Ref<VulkanReflectionProbe> ReflectionProbeGenerator::FromCubemap(const Ref<VulkanCubeMapTexture>& cubemap)
    {
        if (!brdfLUT) GenerateBrdfLUT();

        constexpr uint32_t PREFILTER_MIP_LEVELS = 6;
        uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(128, 128)))) + 1;

        Ref<VulkanCubeMapTexture> prefilterMap = VulkanCubeMapTexture::Builder()
                .SetFormat(ImageFormat::RGBA16_SFLOAT)
                .SetResolution(128, 128)
                .SetMipLevels(mipLevels)
                .Build(device);

        std::vector<std::vector<Ref<VulkanFramebuffer>>> iblPrefilterFramebuffes;
        iblPrefilterFramebuffes.resize(PREFILTER_MIP_LEVELS);

        for (unsigned int mip = 0; mip < PREFILTER_MIP_LEVELS; ++mip)
        {
            const unsigned int mipWidth = static_cast<unsigned int>(128 * std::pow(0.5, mip));
            const unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));

            iblPrefilterFramebuffes[mip].resize(6);
            for (size_t i = 0; i < 6; i++)
            {
                iblPrefilterFramebuffes[mip][i] = VulkanFramebuffer::Builder(device)
                        .SetRenderpass(renderPass)
                        .SetResolution(mipWidth, mipHeight)
                        .AddAttachment(prefilterMap->GetFaceImageView(i, mip))
                        .Build();
            }
        }

        VkDescriptorSet cubemapDescriptorSet;
        VkDescriptorImageInfo info{
            cubemap->GetSampler(),
            cubemap->GetImageView(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        VulkanDescriptorWriter(*ShaderCache::descriptorSetLayouts.cubemapDescriptorSetLayout, device->GetShaderDescriptorPool())
                .WriteImage(0, &info)
                .Build(cubemapDescriptorSet);

        device->ImmediateSubmit([&](VkCommandBuffer commandBuffer) {
            for (unsigned int mip = 0; mip < PREFILTER_MIP_LEVELS; ++mip)
            {
                const float roughness = static_cast<float>(mip) / static_cast<float>(PREFILTER_MIP_LEVELS - 1);
                for (size_t i = 0; i < 6; i++)
                {
                    ComputePrefilterMap(commandBuffer, i, roughness, iblPrefilterFramebuffes[mip][i], cubemapDescriptorSet);
                }
            }
        });

        vkFreeDescriptorSets(device->GetDevice(), device->GetShaderDescriptorPool().GetDescriptorPool(), 1, &cubemapDescriptorSet);

        return CreateRef<VulkanReflectionProbe>(device, prefilterMap, brdfLUT);
    }

    void ReflectionProbeGenerator::LoadPipeline()
    {
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

            iblBrdfPipelineConfig.renderPass = renderPass;
        }
        brdfLutPipeline = VulkanPipeline::Builder().Build(device, iblBrdfPipelineConfig);

        PipelineConfig iblPrefilterPipelineConfig; {
            iblPrefilterPipelineConfig.vertexShaderPath = "shader/spv/cubemap.vert.spv";
            iblPrefilterPipelineConfig.fragmentShaderPath = "shader/spv/prefilter.frag.spv";

            iblPrefilterPipelineConfig.cullMode = PipelineCullMode::Back;
            iblPrefilterPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            iblPrefilterPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

            iblPrefilterPipelineConfig.descriptorSetLayouts = {
                ShaderCache::descriptorSetLayouts.cubemapDescriptorSetLayout,
            };

            iblPrefilterPipelineConfig.colorAttachments = {
                ImageFormat::RGBA16_SFLOAT,
            };

            iblPrefilterPipelineConfig.disableBlending = true;
            iblPrefilterPipelineConfig.enableDepthTest = false;

            iblPrefilterPipelineConfig.renderPass = renderPass;
        }
        prefilterPipeline = VulkanPipeline::Builder().Build(device, iblPrefilterPipelineConfig);
    }

    void ReflectionProbeGenerator::GenerateBrdfLUT()
    {
        brdfLUT = VulkanTexture::Builder()
                .SetResolution(512, 512)
                .SetFormat(ImageFormat::RGBA16_SFLOAT)
                .AddAspectFlag(VK_IMAGE_ASPECT_COLOR_BIT)
                .AddUsage(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
                .Build(device);

        auto iblBRDFFramebuffer = VulkanFramebuffer::Builder(device)
                .SetRenderpass(renderPass)
                .SetResolution(512, 512)
                .AddAttachment(brdfLUT->GetImageView())
                .Build();

        device->ImmediateSubmit([&](VkCommandBuffer commandBuffer) {
            ComputeIblBRDF(commandBuffer, iblBRDFFramebuffer);
        });
    }

    void ReflectionProbeGenerator::ComputeIblBRDF(VkCommandBuffer commandBuffer, Ref<VulkanFramebuffer> framebuffer)
    {
        VkExtent2D extent = {framebuffer->width, framebuffer->height};

        device->SetViewportAndScissor(extent, commandBuffer);
        renderPass->Begin(commandBuffer, framebuffer, extent);

        DrawCommandParams drawCommandParams{};
        drawCommandParams.commandBuffer = commandBuffer;

        drawCommandParams.pipelineParams =
        {
            brdfLutPipeline->GetPipeline(),
            brdfLutPipeline->GetPipelineLayout()
        };

        drawCommandParams.meshlet = screenRect.get();

        device->DrawMeshlet(drawCommandParams);

        renderPass->End(commandBuffer);
    }

    void ReflectionProbeGenerator::ComputePrefilterMap(VkCommandBuffer commandBuffer, size_t faceIndex, float roughness,
                                                       Ref<VulkanFramebuffer> framebuffer, VkDescriptorSet cubemapDescriptorSet)
    {
        VkExtent2D extent = {
            framebuffer->width,
            framebuffer->height
        };

        device->SetViewportAndScissor(extent, commandBuffer);
        renderPass->Begin(commandBuffer, framebuffer, extent);

        DrawCommandParams drawCommandParams{};
        drawCommandParams.commandBuffer = commandBuffer;

        drawCommandParams.pipelineParams = {
            prefilterPipeline->GetPipeline(),
            prefilterPipeline->GetPipelineLayout()
        };

        drawCommandParams.meshlet = &cubeMesh->GetMeshlets()[0];

        drawCommandParams.descriptorSets = {
            cubemapDescriptorSet
        };

        PrefilterData pushConstantData;
        pushConstantData.projection = m_CaptureProjection;
        pushConstantData.view = m_CaptureViews[faceIndex];
        pushConstantData.roughness = roughness;

        drawCommandParams.pushConstantParams = {
            &pushConstantData,
            sizeof(PrefilterData)
        };

        device->DrawMeshlet(drawCommandParams);

        renderPass->End(commandBuffer);
    }
}
