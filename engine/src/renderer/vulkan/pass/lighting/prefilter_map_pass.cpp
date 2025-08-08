#include "renderer/vulkan/pass/lighting/prefilter_map_pass.h"

#include <renderer/shader_cache.h>
#include <renderer/vulkan/vulkan_framebuffer.h>
#include <renderer/vulkan/vulkan_mesh.h>
#include <renderer/vulkan/vulkan_texture.h>
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
        : FrameGraphRenderPass(vulkanDevice, _resolution)
    {
        cubeMesh = ResourceManager::LoadMesh(device, "resources/models/cube.obj");
    }

    void PrefilterMapPass::Init()
    {
        FrameGraphRenderPass::Init();
    }

    void PrefilterMapPass::CreateFramebuffer()
    {
        const TextureHandle outputTextureHandle = outputs[0].resource.resourceInfo.texture.textureHandle;
        const VulkanTexture* outputTexture = device->GetTexture(outputTextureHandle);

        for (unsigned int mip = 0; mip < PREFILTER_MIP_LEVELS; ++mip)
        {
            const VkExtent2D extent = {
                static_cast<unsigned int>(REFLECTION_RESOLUTION * std::pow(0.5, mip)),
                static_cast<unsigned int>(REFLECTION_RESOLUTION * std::pow(0.5, mip))
            };
            for (uint32_t faceIndex = 0; faceIndex < 6; faceIndex++)
            {
                FramebufferCreateInfo createInfo = {};
                createInfo.attachments.push_back({.imageView = outputTexture->GetMipmapImageView(mip, faceIndex)});
                createInfo.resolution = extent;
                createInfo.renderPassHandle = renderPassHandle;

                FramebufferHandle framebufferHandle = device->CreateFramebuffer(createInfo);
                framebufferHandles.push_back(framebufferHandle);
            }
        }
    }

    void PrefilterMapPass::Render(VkCommandBuffer commandBuffer, Scene* scene)
    {
        const VulkanTexture* cubemap = device->GetTexture(inputs[0].resourceInfo.texture.textureHandle);

        for (unsigned int mip = 0; mip < PREFILTER_MIP_LEVELS; ++mip)
        {
            const float roughness = static_cast<float>(mip) / static_cast<float>(PREFILTER_MIP_LEVELS - 1);
            for (uint32_t faceIndex = 0; faceIndex < 6; faceIndex++)
            {
                const VulkanFramebuffer* framebuffer = device->GetFramebuffer(framebufferHandles[mip * 6 + faceIndex]);

                device->SetViewportAndScissor(framebuffer->extent, commandBuffer);
                GetRenderPass()->Begin(commandBuffer, framebuffer->framebuffer, framebuffer->extent);

                DrawCommandParams drawCommandParams{};
                drawCommandParams.commandBuffer = commandBuffer;
                drawCommandParams.pipelineHandle = pipelineHandle;
                drawCommandParams.meshlet = &cubeMesh->GetMeshlets()[0];
                drawCommandParams.descriptorSets = {passDescriptorSet};

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
            }
        }
    }

    void PrefilterMapPass::Resize(VkExtent2D _resolution)
    {
        FrameGraphRenderPass::Resize(_resolution);
    }

    void PrefilterMapPass::SetCubemapTexture(TextureHandle _cubemapTextureHandle)
    {
        cubemapTextureHandle = _cubemapTextureHandle;
    }

    void PrefilterMapPass::SetTargetTexture(TextureHandle _targetTexture)
    {
        targetTexture = _targetTexture;
    }

    void PrefilterMapPass::LoadPipeline(PipelineCreateInfo& pipelineCreate)
    {
        pipelineCreate.name = "PrefilterMapPass";
        pipelineCreate.vertexShaderPath = "cubemap.vert";
        pipelineCreate.fragmentShaderPath = "prefilter.frag";

        pipelineCreate.descriptorSetLayouts = {
            passDescriptorSetLayoutHandle,
        };

        pipelineCreate.enableDepthTest = false;
        pipelineCreate.pushConstantData = {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PrefilterData)};

        LOG_TRACE(pipelineCreate.name);
    }

    void PrefilterMapPass::SetRoughness(float _roughness)
    {
        roughness = _roughness;
    }
}
