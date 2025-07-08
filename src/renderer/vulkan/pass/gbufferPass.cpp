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
        renderPass = VulkanRenderPass::Builder(vulkanDevice)
                .AddColorAttachment({.imageFormat = VK_FORMAT_R32G32B32A32_SFLOAT})
                .AddColorAttachment({.imageFormat = VK_FORMAT_R32G32B32A32_SFLOAT})
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

    void GBufferPass::ExecuteWithRenderGraph(VkCommandBuffer cmd, const std::unordered_map<std::string, RenderResource*>& resources)
    {}

    void GBufferPass::LoadPipelines()
    {
        LOG_TRACE("Building gbuffer pipeline");

        constexpr PipelinePushConstantData pushConstantData = {
            .shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = sizeof(SimplePushConstantData),
        };

        PipelineConfig pipelineConfig = {
            .vertexShaderPath = "gbuffer.vert",
            .fragmentShaderPath = "gbuffer.frag",

            .descriptorSetLayouts = {
                device->bindlessDescriptorSetLayout,
                ShaderCache::descriptorSetLayouts.materialDescriptorSetLayout,
                ShaderCache::descriptorSetLayouts.transformDescriptorSetLayout,
            },

            .colorAttachments = {
                ImageFormat::RGBA32_SFLOAT,
                ImageFormat::RGBA32_SFLOAT,
            },
            .depthAttachment = ImageFormat::DEPTH32,

            .polygonMode = PipelinePolygonMode::Fill,
            .frontFace = PipelineFrontFace::Counter_clockwise,
            .cullMode = PipelineCullMode::Back,

            .renderPass = renderPass,

            .pushConstantData = pushConstantData,

            .enableDepthTest = true,
            .disableBlending = true,
        };

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

        const VkDescriptorImageInfo worldSpaceNormalInfo{
            .sampler = gBuffer->buffers.viewSpaceNormal.sampler,
            .imageView = gBuffer->buffers.viewSpaceNormal.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        const VkDescriptorImageInfo positionInfo{
            .sampler = gBuffer->buffers.viewSpacePosition.sampler,
            .imageView = gBuffer->buffers.viewSpacePosition.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        const VkDescriptorImageInfo depthInfo{
            .sampler = gBuffer->buffers.depth.sampler,
            .imageView = gBuffer->buffers.depth.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        VulkanDescriptorWriter(*ShaderCache::descriptorSetLayouts.gBufferDescriptorSetLayout, device->GetShaderDescriptorPool())
                .WriteImage(0, &worldSpaceNormalInfo)
                .WriteImage(1, &positionInfo)
                .WriteImage(2, &depthInfo)
                .BuildOrOverwrite(ShaderCache::descriptorSets.gbufferDescriptorSet);
    }
}
