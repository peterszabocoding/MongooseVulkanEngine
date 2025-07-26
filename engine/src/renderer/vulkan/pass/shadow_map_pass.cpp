#include "renderer/vulkan/pass/shadow_map_pass.h"

#include <renderer/vulkan/vulkan_mesh.h>

#include "renderer/shader_cache.h"
#include "util/log.h"

namespace MongooseVK
{
    ShadowMapPass::ShadowMapPass(VulkanDevice* vulkanDevice, Scene& _scene, VkExtent2D _resolution): VulkanPass(vulkanDevice, _resolution),
        scene(_scene)
    {}

    void ShadowMapPass::Init()
    {
        VulkanPass::Init();
    }

    void ShadowMapPass::InitFramebuffer()
    {
        const TextureHandle outputTextureHandle = outputs[0].resource.resourceInfo.texture.textureHandle;
        const VulkanTexture* outputTexture = device->GetTexture(outputTextureHandle);

        for (uint32_t i = 0; i < outputTexture->createInfo.arrayLayers; i++)
        {
            FramebufferCreateInfo framebufferCreateInfo = {
                .attachments = {},
                .renderPassHandle = renderPassHandle,
                .resolution = resolution,
            };

            framebufferCreateInfo.attachments.push_back({.imageView = outputTexture->arrayImageViews[i]});

            framebufferHandles.push_back(device->CreateFramebuffer(framebufferCreateInfo));
        }

    }

    void ShadowMapPass::Render(VkCommandBuffer commandBuffer, Camera* camera, FramebufferHandle writeBuffer)
    {
        for (uint32_t i = 0; i < framebufferHandles.size(); i++)
        {
            VulkanFramebuffer* framebuffer = device->GetFramebuffer(framebufferHandles[i]);

            device->SetViewportAndScissor(framebuffer->extent, commandBuffer);
            GetRenderPass()->Begin(commandBuffer, framebuffer->framebuffer, framebuffer->extent);

            DrawCommandParams geometryDrawParams{};
            geometryDrawParams.commandBuffer = commandBuffer;
            geometryDrawParams.pipelineParams = {
                pipeline->pipeline,
                pipeline->pipelineLayout
            };

            SimplePushConstantData pushConstantData;

            pushConstantData.transform = scene.directionalLight.cascades[i].viewProjMatrix;

            for (size_t i = 0; i < scene.meshes.size(); i++)
            {
                pushConstantData.modelMatrix = scene.transforms[i].GetTransform();

                geometryDrawParams.pushConstantParams = {
                    &pushConstantData,
                    sizeof(SimplePushConstantData)
                };

                for (auto& meshlet: scene.meshes[i]->GetMeshlets())
                {
                    geometryDrawParams.meshlet = &meshlet;
                    device->DrawMeshlet(geometryDrawParams);
                }
            }

            GetRenderPass()->End(commandBuffer);
        }
    }

    void ShadowMapPass::LoadPipeline()
    {
        LOG_TRACE("Building directional shadow map pipeline");
        pipelineConfig.vertexShaderPath = "depth_only.vert";
        pipelineConfig.fragmentShaderPath = "empty.frag";

        pipelineConfig.cullMode = PipelineCullMode::Front;
        pipelineConfig.polygonMode = PipelinePolygonMode::Fill;
        pipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

        pipelineConfig.disableBlending = true;
        pipelineConfig.enableDepthTest = true;

        pipelineConfig.renderPass = GetRenderPass()->Get();

        pipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineConfig.pushConstantData.offset = 0;
        pipelineConfig.pushConstantData.size = sizeof(SimplePushConstantData);

        pipelineHandle = VulkanPipelineBuilder().Build(device, pipelineConfig);
        pipeline = device->GetPipeline(pipelineHandle);
    }
}
