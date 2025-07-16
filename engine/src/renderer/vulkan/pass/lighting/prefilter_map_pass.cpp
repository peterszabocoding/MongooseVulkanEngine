#include "renderer/vulkan/pass/lighting/prefilter_map_pass.h"

#include <renderer/shader_cache.h>
#include <renderer/vulkan/vulkan_mesh.h>
#include <resource/resource_manager.h>

namespace MongooseVK
{
    constexpr uint32_t PREFILTER_MIP_LEVELS = 6;
    constexpr uint32_t REFLECTION_RESOLUTION = 256;

    const glm::mat4 m_CaptureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    const glm::mat4 m_CaptureViews[6] = {
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
    };

    PrefilterMapPass::PrefilterMapPass(VulkanDevice* vulkanDevice, const VkExtent2D& _resolution)
        : VulkanPass(vulkanDevice, _resolution)
    {
        cubeMesh = ResourceManager::LoadMesh(device, "resources/models/cube.obj");
        LoadPipeline();
    }

    void PrefilterMapPass::Render(VkCommandBuffer commandBuffer, Camera* camera, FramebufferHandle writeBuffer)
    {
        VulkanTexture* prefilterMap = device->GetTexture(targetTexture);
        VulkanTexture* cubemap = device->GetTexture(cubemapTextureHandle);

        for (unsigned int mip = 0; mip < PREFILTER_MIP_LEVELS; ++mip)
        {
            const float roughness = static_cast<float>(mip) / static_cast<float>(PREFILTER_MIP_LEVELS - 1);
            const VkExtent2D extent = {
                static_cast<unsigned int>(REFLECTION_RESOLUTION * std::pow(0.5, mip)),
                static_cast<unsigned int>(REFLECTION_RESOLUTION * std::pow(0.5, mip))
            };

            for (uint32_t faceIndex = 0; faceIndex < 6; faceIndex++)
            {
                FramebufferCreateInfo createInfo = {};
                createInfo.attachments.push_back({.imageView = prefilterMap->GetMipmapImageView(mip, faceIndex)});
                createInfo.resolution = extent;
                createInfo.renderPassHandle = GetRenderPassHandle();

                FramebufferHandle framebufferHandle = device->CreateFramebuffer(createInfo);
                VulkanFramebuffer* framebuffer = device->GetFramebuffer(framebufferHandle);

                device->SetViewportAndScissor(extent, commandBuffer);
                GetRenderPass()->Begin(commandBuffer, framebuffer->framebuffer, extent);

                DrawCommandParams drawCommandParams{};
                drawCommandParams.commandBuffer = commandBuffer;

                drawCommandParams.pipelineParams = {
                    prefilterMapPipeline->GetPipeline(),
                    prefilterMapPipeline->GetPipelineLayout()
                };

                drawCommandParams.meshlet = &cubeMesh->GetMeshlets()[0];

                drawCommandParams.descriptorSets = {
                    ShaderCache::descriptorSets.cubemapDescriptorSet
                };

                PrefilterData pushConstantData;
                pushConstantData.projection = m_CaptureProjection;
                pushConstantData.view = m_CaptureViews[faceIndex];
                pushConstantData.roughness = roughness;
                pushConstantData.resolution = cubemap->createInfo.width;

                drawCommandParams.pushConstantParams = {
                    &pushConstantData,
                    sizeof(PrefilterData),
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                };

                device->DrawMeshlet(drawCommandParams);

                GetRenderPass()->End(commandBuffer);

                device->DestroyFramebuffer(framebufferHandle);
            }
        }
    }

    void PrefilterMapPass::Resize(VkExtent2D _resolution)
    {
        VulkanPass::Resize(_resolution);
    }

    void PrefilterMapPass::SetCubemapTexture(TextureHandle _cubemapTextureHandle)
    {
        cubemapTextureHandle = _cubemapTextureHandle;
    }

    void PrefilterMapPass::SetTargetTexture(TextureHandle _targetTexture)
    {
        targetTexture = _targetTexture;
    }

    void PrefilterMapPass::LoadPipeline()
    {
        VulkanRenderPass::RenderPassConfig config;
        config.AddColorAttachment({.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT});

        renderPassHandle = device->CreateRenderPass(config);

        PipelineConfig iblPrefilterPipelineConfig;
        iblPrefilterPipelineConfig.vertexShaderPath = "cubemap.vert";
        iblPrefilterPipelineConfig.fragmentShaderPath = "prefilter.frag";

        iblPrefilterPipelineConfig.descriptorSetLayouts = {
            ShaderCache::descriptorSetLayouts.cubemapDescriptorSetLayout,
        };

        iblPrefilterPipelineConfig.colorAttachments = {
            ImageFormat::RGBA16_SFLOAT,
        };

        iblPrefilterPipelineConfig.enableDepthTest = false;

        iblPrefilterPipelineConfig.pushConstantData = {
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(PrefilterData)
        };

        iblPrefilterPipelineConfig.renderPass = GetRenderPass()->Get();

        prefilterMapPipeline = VulkanPipeline::Builder().Build(device, iblPrefilterPipelineConfig);
    }

    void PrefilterMapPass::SetRoughness(float _roughness)
    {
        roughness = _roughness;
    }
}
