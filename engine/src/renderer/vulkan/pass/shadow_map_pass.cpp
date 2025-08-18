#include "renderer/vulkan/pass/shadow_map_pass.h"

#include <renderer/vulkan/vulkan_framebuffer.h>
#include <renderer/vulkan/vulkan_mesh.h>
#include <renderer/vulkan/vulkan_texture.h>

#include "renderer/shader_cache.h"
#include "util/log.h"

namespace MongooseVK
{
    ShadowMapPass::ShadowMapPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution): FrameGraphRenderPass(
        vulkanDevice, VkExtent2D{SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION}) {}

    void ShadowMapPass::Setup(FrameGraph* frameGraph)
    {
        FrameGraphResourceCreate resourceCreate{};
        resourceCreate.name = "directional_shadow_map";
        resourceCreate.type = FrameGraphResourceType::Texture;
        resourceCreate.resourceInfo.texture.textureCreateInfo = {};
        resourceCreate.resourceInfo.texture.textureCreateInfo.width = resolution.width;
        resourceCreate.resourceInfo.texture.textureCreateInfo.height = resolution.height;
        resourceCreate.resourceInfo.texture.textureCreateInfo.format = ImageFormat::DEPTH32;
        resourceCreate.resourceInfo.texture.textureCreateInfo.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        resourceCreate.resourceInfo.texture.textureCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        resourceCreate.resourceInfo.texture.textureCreateInfo.arrayLayers = SHADOW_MAP_CASCADE_COUNT;
        resourceCreate.resourceInfo.texture.textureCreateInfo.compareEnabled = true;
        resourceCreate.resourceInfo.texture.textureCreateInfo.compareOp = VK_COMPARE_OP_LESS;

        frameGraph->CreateResource(resourceCreate);
    }

    void ShadowMapPass::Init()
    {
        FrameGraphRenderPass::Init();
    }

    void ShadowMapPass::CreateFramebuffer()
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

    void ShadowMapPass::Render(VkCommandBuffer commandBuffer, Scene* scene)
    {
        for (uint32_t i = 0; i < framebufferHandles.size(); i++)
        {
            VulkanFramebuffer* framebuffer = device->GetFramebuffer(framebufferHandles[i]);

            device->SetViewportAndScissor(framebuffer->extent, commandBuffer);
            GetRenderPass()->Begin(commandBuffer, framebuffer->framebuffer, framebuffer->extent);

            DrawCommandParams geometryDrawParams{};
            geometryDrawParams.commandBuffer = commandBuffer;
            geometryDrawParams.pipelineHandle = pipelineHandle;

            ShadowMapPushConstantData pushConstantData;

            pushConstantData.projection = scene->directionalLight.cascades[i].viewProjMatrix;

            for (size_t i = 0; i < scene->meshes.size(); i++)
            {
                pushConstantData.modelMatrix = scene->transforms[i].GetTransform();

                geometryDrawParams.pushConstantParams = {
                    &pushConstantData,
                    sizeof(ShadowMapPushConstantData)
                };

                for (auto& meshlet: scene->meshes[i]->GetMeshlets())
                {
                    geometryDrawParams.meshlet = &meshlet;
                    device->DrawMeshlet(geometryDrawParams);
                }
            }

            GetRenderPass()->End(commandBuffer);
        }
    }

    void ShadowMapPass::Resize(VkExtent2D _resolution) {}

    void ShadowMapPass::LoadPipeline(PipelineCreateInfo& pipelineCreate)
    {
        pipelineCreate.name = "ShadowMapPass";
        pipelineCreate.vertexShaderPath = "depth_only.vert";
        pipelineCreate.fragmentShaderPath = "empty.frag";

        pipelineCreate.cullMode = PipelineCullMode::Front;

        pipelineCreate.disableBlending = true;
        pipelineCreate.enableDepthTest = true;

        pipelineCreate.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineCreate.pushConstantData.size = sizeof(ShadowMapPushConstantData);
    }
}
