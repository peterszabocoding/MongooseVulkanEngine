#include "renderer/vulkan/lighting/reflection_probe_generator.h"

#include "renderer/shader_cache.h"
#include "renderer/vulkan/vulkan_descriptor_writer.h"
#include "renderer/vulkan/vulkan_framebuffer.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "resource/resource_manager.h"

namespace MongooseVK
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
        LoadPipeline();

        screenRect = CreateScope<VulkanMeshlet>(device, Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);
        cubeMesh = ResourceManager::LoadMesh(device, "resources/models/cube.obj");
    }

    Ref<VulkanReflectionProbe> ReflectionProbeGenerator::FromCubemap(TextureHandle cubemapTextureHandle)
    {
        VulkanTexture* cubemap = device->GetTexture(cubemapTextureHandle);

        constexpr uint32_t PREFILTER_MIP_LEVELS = 6;
        constexpr uint32_t REFLECTION_RESOLUTION = 256;

        TextureCreateInfo textureCreateInfo = {};
        textureCreateInfo.width = REFLECTION_RESOLUTION;
        textureCreateInfo.height = REFLECTION_RESOLUTION;
        textureCreateInfo.format = ImageFormat::RGBA16_SFLOAT;
        textureCreateInfo.imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA16_SFLOAT);
        textureCreateInfo.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        textureCreateInfo.mipLevels = PREFILTER_MIP_LEVELS;
        textureCreateInfo.arrayLayers = 6;
        textureCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        textureCreateInfo.isCubeMap = true;

        TextureHandle prefilterTextureMap = device->CreateTexture(textureCreateInfo);
        VulkanTexture* prefilterMap = device->GetTexture(prefilterTextureMap);

        const Ref<VulkanFramebuffer> framebuffer = VulkanFramebuffer::Builder(device)
                .SetRenderpass(GetRenderPass())
                .SetResolution(REFLECTION_RESOLUTION, REFLECTION_RESOLUTION)
                .AddAttachment(ImageFormat::RGBA16_SFLOAT)
                .Build();

        device->ImmediateSubmit([&](const VkCommandBuffer commandBuffer) {
            VulkanUtils::TransitionImageLayout(commandBuffer,
                                               prefilterMap->GetImage(),
                                               VK_IMAGE_ASPECT_COLOR_BIT,
                                               VK_IMAGE_LAYOUT_UNDEFINED,
                                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            for (unsigned int mip = 0; mip < PREFILTER_MIP_LEVELS; ++mip)
            {
                const float roughness = static_cast<float>(mip) / static_cast<float>(PREFILTER_MIP_LEVELS - 1);
                const VkExtent2D extent = {
                    static_cast<unsigned int>(REFLECTION_RESOLUTION * std::pow(0.5, mip)),
                    static_cast<unsigned int>(REFLECTION_RESOLUTION * std::pow(0.5, mip))
                };

                for (uint32_t faceIndex = 0; faceIndex < 6; faceIndex++)
                {
                    ComputePrefilterMap(commandBuffer, extent, faceIndex, roughness, framebuffer, cubemap->createInfo.width);

                    const VulkanUtils::CopyParams params = {
                        .srcMipLevel = 0,
                        .dstMipLevel = mip,
                        .srcBaseArrayLayer = 0,
                        .dstBaseArrayLayer = faceIndex,
                        .regionWidth = extent.width,
                        .regionHeight = extent.height,
                    };

                    CopyImage(commandBuffer,
                              framebuffer->GetAttachments()[0].allocatedImage.image,
                              prefilterMap->GetImage(),
                              params);
                }
            }

            VulkanUtils::TransitionImageLayout(commandBuffer,
                                               prefilterMap->GetImage(),
                                               VK_IMAGE_ASPECT_COLOR_BIT,
                                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        });

        return CreateRef<VulkanReflectionProbe>(device, prefilterTextureMap);
    }

    VulkanRenderPass* ReflectionProbeGenerator::GetRenderPass()
    {
        return device->renderPassPool.Get(renderPassHandle.handle);
    }


    void ReflectionProbeGenerator::LoadPipeline()
    {
        VulkanRenderPass::RenderPassConfig config;
        config.AddColorAttachment({.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT});

        renderPassHandle = device->CreateRenderPass(config);

        PipelineConfig iblPrefilterPipelineConfig;
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

        prefilterPipeline = VulkanPipeline::Builder().Build(device, iblPrefilterPipelineConfig);
    }

    void ReflectionProbeGenerator::ComputePrefilterMap(const VkCommandBuffer commandBuffer,
                                                       const VkExtent2D extent,
                                                       const size_t faceIndex,
                                                       const float roughness,
                                                       const Ref<VulkanFramebuffer>& framebuffer,
                                                       uint32_t resolution)
    {
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
            ShaderCache::descriptorSets.cubemapDescriptorSet
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
