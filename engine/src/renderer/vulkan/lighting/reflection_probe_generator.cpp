//
// Created by peter on 2025. 05. 18..
//

#include "renderer/vulkan/lighting/reflection_probe_generator.h"

#include "renderer/shader_cache.h"
#include "renderer/vulkan/vulkan_descriptor_writer.h"
#include "renderer/vulkan/vulkan_framebuffer.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "resource/resource_manager.h"

namespace MongooseVK {
    const glm::mat4 m_CaptureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    const glm::mat4 m_CaptureViews[6] = {
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
    };

    ReflectionProbeGenerator::ReflectionProbeGenerator(VulkanDevice* _device): device(_device) {
        VulkanRenderPass::RenderPassConfig config;
        config.AddColorAttachment({.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT});

        renderPassHandle = device->CreateRenderPass(config);

        LoadPipeline();

        screenRect = CreateScope<VulkanMeshlet>(device, Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);
        cubeMesh = ResourceManager::LoadMesh(device, "resources/models/cube.obj");
    }

    Ref<VulkanReflectionProbe> ReflectionProbeGenerator::FromCubemap(const Ref<VulkanCubeMapTexture>& cubemap) {
        if (!brdfLUT) GenerateBrdfLUT();

        constexpr uint32_t PREFILTER_MIP_LEVELS = 6;
        constexpr uint32_t REFLECTION_RESOLUTION = 128;

        uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(REFLECTION_RESOLUTION, REFLECTION_RESOLUTION)))) + 1;

        Ref<VulkanCubeMapTexture> prefilterMap = VulkanCubeMapTexture::Builder()
                .SetFormat(ImageFormat::RGBA16_SFLOAT)
                .SetResolution(REFLECTION_RESOLUTION, REFLECTION_RESOLUTION)
                .SetMipLevels(mipLevels)
                .Build(device);


        device->ImmediateSubmit([&](const VkCommandBuffer commandBuffer) {
            VulkanUtils::TransitionImageLayout(commandBuffer,
                                               prefilterMap->GetImage(),
                                               VK_IMAGE_ASPECT_COLOR_BIT,
                                               VK_IMAGE_LAYOUT_UNDEFINED,
                                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        });

        Ref<VulkanFramebuffer> framebuffer = VulkanFramebuffer::Builder(device)
                .SetRenderpass(GetRenderPass())
                .SetResolution(REFLECTION_RESOLUTION, REFLECTION_RESOLUTION)
                .AddAttachment(ImageFormat::RGBA16_SFLOAT)
                .Build();

        VkDescriptorSet cubemapDescriptorSet;
        VkDescriptorImageInfo info{
            cubemap->GetSampler(),
            cubemap->GetImageView(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        VulkanDescriptorWriter(*ShaderCache::descriptorSetLayouts.cubemapDescriptorSetLayout, device->GetShaderDescriptorPool())
                .WriteImage(0, &info)
                .Build(cubemapDescriptorSet);


        for (unsigned int mip = 0; mip < PREFILTER_MIP_LEVELS; ++mip) {
            const float roughness = static_cast<float>(mip) / static_cast<float>(PREFILTER_MIP_LEVELS - 1);
            const VkExtent2D extent = {
                static_cast<unsigned int>(REFLECTION_RESOLUTION * std::pow(0.5, mip)),
                static_cast<unsigned int>(REFLECTION_RESOLUTION * std::pow(0.5, mip))
            };

            for (size_t faceIndex = 0; faceIndex < 6; faceIndex++) {
                device->ImmediateSubmit([&](const VkCommandBuffer commandBuffer) {
                    ComputePrefilterMap(commandBuffer, extent, faceIndex, roughness, framebuffer, cubemapDescriptorSet, cubemap->GetWidth());

                    VulkanUtils::CopyParams params{};
                    params.srcMipLevel = 0;
                    params.dstMipLevel = mip;
                    params.srcBaseArrayLayer = 0;
                    params.dstBaseArrayLayer = faceIndex;
                    params.regionWidth = extent.width;
                    params.regionHeight = extent.height;

                    CopyImage(commandBuffer,
                              framebuffer->GetAttachments()[0].allocatedImage.image,
                              prefilterMap->GetImage(),
                              params);
                });
            }
        }

        device->ImmediateSubmit([&](const VkCommandBuffer commandBuffer) {
            VulkanUtils::TransitionImageLayout(commandBuffer,
                                               prefilterMap->GetImage(),
                                               VK_IMAGE_ASPECT_COLOR_BIT,
                                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        });

        vkFreeDescriptorSets(device->GetDevice(), device->GetShaderDescriptorPool().GetDescriptorPool(), 1, &cubemapDescriptorSet);

        return CreateRef<VulkanReflectionProbe>(device, prefilterMap, brdfLUT);
    }

    VulkanRenderPass* ReflectionProbeGenerator::GetRenderPass()
    {
        return device->renderPassPool.Get(renderPassHandle.handle);
    }


    void ReflectionProbeGenerator::LoadPipeline() {
        PipelineConfig iblBrdfPipelineConfig; {
            iblBrdfPipelineConfig.vertexShaderPath = "brdf.vert";
            iblBrdfPipelineConfig.fragmentShaderPath = "brdf.frag";

            iblBrdfPipelineConfig.cullMode = PipelineCullMode::Front;
            iblBrdfPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            iblBrdfPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

            iblBrdfPipelineConfig.descriptorSetLayouts = {};

            iblBrdfPipelineConfig.colorAttachments = {
                ImageFormat::RGBA16_SFLOAT,
            };

            iblBrdfPipelineConfig.disableBlending = true;
            iblBrdfPipelineConfig.enableDepthTest = false;

            iblBrdfPipelineConfig.renderPass = GetRenderPass()->Get();
        }
        brdfLutPipeline = VulkanPipeline::Builder().Build(device, iblBrdfPipelineConfig);

        PipelineConfig iblPrefilterPipelineConfig; {
            iblPrefilterPipelineConfig.vertexShaderPath = "cubemap.vert";
            iblPrefilterPipelineConfig.fragmentShaderPath = "prefilter.frag";

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

            iblPrefilterPipelineConfig.pushConstantData = {
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(PrefilterData)
            };

            iblPrefilterPipelineConfig.renderPass = GetRenderPass()->Get();
        }
        prefilterPipeline = VulkanPipeline::Builder().Build(device, iblPrefilterPipelineConfig);
    }

    void ReflectionProbeGenerator::GenerateBrdfLUT() {
        brdfLUT = VulkanTextureBuilder()
                .SetResolution(512, 512)
                .SetFormat(ImageFormat::RGBA16_SFLOAT)
                .AddAspectFlag(VK_IMAGE_ASPECT_COLOR_BIT)
                .AddUsage(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
                .Build(device);

        auto iblBRDFFramebuffer = VulkanFramebuffer::Builder(device)
                .SetRenderpass(GetRenderPass())
                .SetResolution(512, 512)
                .AddAttachment(brdfLUT->GetImageView())
                .Build();

        device->ImmediateSubmit([&](const VkCommandBuffer commandBuffer) {
            ComputeIblBRDF(commandBuffer, iblBRDFFramebuffer);
        });
    }

    void ReflectionProbeGenerator::ComputeIblBRDF(const VkCommandBuffer commandBuffer, const Ref<VulkanFramebuffer>& framebuffer) {
        VkExtent2D extent = {framebuffer->GetWidth(), framebuffer->GetHeight()};

        device->SetViewportAndScissor(extent, commandBuffer);
        GetRenderPass()->Begin(commandBuffer, framebuffer, extent);

        DrawCommandParams drawCommandParams{};
        drawCommandParams.commandBuffer = commandBuffer;

        drawCommandParams.pipelineParams =
        {
            brdfLutPipeline->GetPipeline(),
            brdfLutPipeline->GetPipelineLayout()
        };

        drawCommandParams.meshlet = screenRect.get();

        device->DrawMeshlet(drawCommandParams);

        GetRenderPass()->End(commandBuffer);
    }

    void ReflectionProbeGenerator::ComputePrefilterMap(const VkCommandBuffer commandBuffer,
                                                       const VkExtent2D extent,
                                                       const size_t faceIndex,
                                                       const float roughness,
                                                       const Ref<VulkanFramebuffer>& framebuffer,
                                                       VkDescriptorSet cubemapDescriptorSet,
                                                       uint32_t resolution) {
        device->SetViewportAndScissor(extent, commandBuffer);
        GetRenderPass()->Begin(commandBuffer, framebuffer, extent);

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
        pushConstantData.resolution = resolution;

        drawCommandParams.pushConstantParams = {
            &pushConstantData,
            sizeof(PrefilterData),
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        };

        device->DrawMeshlet(drawCommandParams);

        GetRenderPass()->End(commandBuffer);
    }
}
