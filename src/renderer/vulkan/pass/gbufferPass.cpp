#include "gbufferPass.h"

#include <random>
#include <glm/gtc/packing.inl>

#include "render_pass.h"
#include "renderer/shader_cache.h"
#include "renderer/vulkan/vulkan_descriptor_writer.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "util/log.h"

namespace Raytracing
{
    GBufferPass::GBufferPass(VulkanDevice* vulkanDevice, Scene& _scene, VkExtent2D _resolution): VulkanPass(vulkanDevice, _resolution),
        scene(_scene)
    {
        VulkanRenderPass::ColorAttachment colorAttachment;
        colorAttachment.imageFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

        renderPass = VulkanRenderPass::Builder(vulkanDevice)
                .AddColorAttachment(colorAttachment)
                .AddColorAttachment(colorAttachment)
                .AddDepthAttachment(VK_FORMAT_D32_SFLOAT)
                .Build();
        LoadPipelines();

        BuildGBuffer();
        CreateFramebuffer();
    }

    void GBufferPass::Render(VkCommandBuffer commandBuffer, Camera& camera, Ref<VulkanFramebuffer> writeBuffer,
                             Ref<VulkanFramebuffer> readBuffer)
    {
        device->SetViewportAndScissor(resolution, commandBuffer);
        renderPass->Begin(commandBuffer, framebuffer, resolution);

        DrawCommandParams geometryDrawParams{};
        geometryDrawParams.commandBuffer = commandBuffer;
        geometryDrawParams.pipelineParams =
        {
            gbufferPipeline->GetPipeline(),
            gbufferPipeline->GetPipelineLayout()
        };

        for (size_t i = 0; i < scene.meshes.size(); i++)
        {
            SimplePushConstantData pushConstantData;
            pushConstantData.modelMatrix = scene.transforms[i].GetTransform();
            pushConstantData.transform = camera.GetProjection() * camera.GetView() * pushConstantData.modelMatrix;

            geometryDrawParams.pushConstantParams = {
                &pushConstantData,
                sizeof(SimplePushConstantData)
            };

            for (auto& meshlet: scene.meshes[i]->GetMeshlets())
            {
                if (scene.meshes[i]->GetMaterial(meshlet).params.alphaTested) continue;

                geometryDrawParams.descriptorSets = {
                    device->bindlessTextureDescriptorSet,
                    scene.meshes[i]->GetMaterial(meshlet).descriptorSet,
                    ShaderCache::descriptorSets.transformDescriptorSet,
                };

                geometryDrawParams.meshlet = &meshlet;
                device->DrawMeshlet(geometryDrawParams);
            }
        }

        renderPass->End(commandBuffer);
    }

    void GBufferPass::Resize(VkExtent2D _resolution)
    {
        VulkanPass::Resize(_resolution);
        BuildGBuffer();
        CreateFramebuffer();
    }

    void GBufferPass::LoadPipelines()
    {
        LOG_TRACE("Building gbuffer pipeline");
        PipelineConfig pipelineConfig; {
            pipelineConfig.vertexShaderPath = "gbuffer.vert";
            pipelineConfig.fragmentShaderPath = "gbuffer.frag";

            pipelineConfig.cullMode = PipelineCullMode::Back;
            pipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            pipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

            pipelineConfig.descriptorSetLayouts = {
                device->bindlessDescriptorSetLayout,
                ShaderCache::descriptorSetLayouts.materialDescriptorSetLayout,
                ShaderCache::descriptorSetLayouts.transformDescriptorSetLayout,
            };

            pipelineConfig.colorAttachments = {
                ImageFormat::RGBA32_SFLOAT,
                ImageFormat::RGBA32_SFLOAT,
            };

            pipelineConfig.disableBlending = true;
            pipelineConfig.enableDepthTest = true;
            pipelineConfig.depthAttachment = ImageFormat::DEPTH32;

            pipelineConfig.renderPass = renderPass;

            pipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            pipelineConfig.pushConstantData.offset = 0;
            pipelineConfig.pushConstantData.size = sizeof(SimplePushConstantData);
        }
        gbufferPipeline = VulkanPipeline::Builder().Build(device, pipelineConfig);
    }

    void GBufferPass::CreateFramebuffer()
    {
        framebuffer = VulkanFramebuffer::Builder(device)
                .SetRenderpass(renderPass)
                .SetResolution(resolution.width, resolution.height)
                .AddAttachment(gBuffer->buffers.viewSpaceNormal.imageView)
                .AddAttachment(gBuffer->buffers.viewSpacePosition.imageView)
                .AddAttachment(gBuffer->buffers.depth.imageView)
                .Build();
    }

    void GBufferPass::BuildGBuffer()
    {
        gBuffer = VulkanGBuffer::Builder()
                .SetResolution(resolution.width, resolution.height)
                .Build(device);

        VkDescriptorImageInfo worldSpaceNormalInfo{};
        worldSpaceNormalInfo.sampler = gBuffer->buffers.viewSpaceNormal.sampler;
        worldSpaceNormalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        worldSpaceNormalInfo.imageView = gBuffer->buffers.viewSpaceNormal.imageView;

        VkDescriptorImageInfo positionInfo{};
        positionInfo.sampler = gBuffer->buffers.viewSpacePosition.sampler;
        positionInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        positionInfo.imageView = gBuffer->buffers.viewSpacePosition.imageView;

        VkDescriptorImageInfo depthInfo{};
        depthInfo.sampler = gBuffer->buffers.depth.sampler;
        depthInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        depthInfo.imageView = gBuffer->buffers.depth.imageView;

        auto writer = VulkanDescriptorWriter(*ShaderCache::descriptorSetLayouts.gBufferDescriptorSetLayout,
                                             device->GetShaderDescriptorPool())
                .WriteImage(0, &worldSpaceNormalInfo)
                .WriteImage(1, &positionInfo)
                .WriteImage(2, &depthInfo);

        if (ShaderCache::descriptorSets.gbufferDescriptorSet)
            writer.Overwrite(ShaderCache::descriptorSets.gbufferDescriptorSet);
        else
            writer.Build(ShaderCache::descriptorSets.gbufferDescriptorSet);
    }
}
