#include "render_pass.h"

#include "renderer/shader_cache.h"
#include "resource/resource_manager.h"

namespace Raytracing
{
    RenderPass::RenderPass(VulkanDevice* vulkanDevice, Scene& scene): VulkanPass(vulkanDevice), scene(scene)
    {
        renderPass = VulkanRenderPass::Builder(vulkanDevice)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM, false)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM, false)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM, false)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM, false)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM, false)
                .AddDepthAttachment()
                .Build();

        cubeMesh = ResourceManager::LoadMesh(device, "resources/models/cube.obj");
    }

    void RenderPass::SetCamera(const Camera& _camera)
    {
        camera = _camera;
    }

    void RenderPass::Render(VkCommandBuffer commandBuffer, uint32_t imageIndex, Ref<VulkanFramebuffer> writeBuffer,
                            Ref<VulkanFramebuffer> readBuffer)
    {
        VkExtent2D extent = {passWidth, passHeight};

        device->SetViewportAndScissor(extent, commandBuffer);
        renderPass->Begin(commandBuffer, writeBuffer, extent);

        DrawSkybox(commandBuffer);

        DrawCommandParams geometryDrawParams{};
        geometryDrawParams.commandBuffer = commandBuffer;
        geometryDrawParams.pipelineParams =
        {
            ShaderCache::pipelines.geometry->GetPipeline(),
            ShaderCache::pipelines.geometry->GetPipelineLayout()
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
                geometryDrawParams.descriptorSets = {
                    scene.meshes[i]->GetMaterial(meshlet).descriptorSet,
                    ShaderCache::descriptorSets.transformDescriptorSet,
                    scene.skybox->descriptorSet,
                    ShaderCache::descriptorSets.lightsDescriptorSets[imageIndex],
                    ShaderCache::descriptorSets.pbrDescriptorSet
                };
                geometryDrawParams.meshlet = &meshlet;
                device->DrawMeshlet(geometryDrawParams);
            }
        }

        renderPass->End(commandBuffer);
    }

    void RenderPass::DrawSkybox(const VkCommandBuffer commandBuffer) const
    {
        DrawCommandParams skyboxDrawParams{};
        skyboxDrawParams.commandBuffer = commandBuffer;
        skyboxDrawParams.meshlet = &cubeMesh->GetMeshlets()[0];

        skyboxDrawParams.pipelineParams = {
            ShaderCache::pipelines.skyBox->GetPipeline(),
            ShaderCache::pipelines.skyBox->GetPipelineLayout()
        };

        skyboxDrawParams.descriptorSets = {
            scene.skybox->descriptorSet,
            ShaderCache::descriptorSets.transformDescriptorSet
        };

        device->DrawMeshlet(skyboxDrawParams);
    }
}
