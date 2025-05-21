#include "render_pass.h"

#include "renderer/shader_cache.h"
#include "resource/resource_manager.h"
#include "util/log.h"

namespace Raytracing
{
    RenderPass::RenderPass(VulkanDevice* vulkanDevice, Scene& _scene): VulkanPass(vulkanDevice), scene(_scene)
    {
        renderPass = VulkanRenderPass::Builder(vulkanDevice)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM, false)
                .AddDepthAttachment()
                .Build();

        cubeMesh = ResourceManager::LoadMesh(device, "resources/models/cube.obj");
        LoadPipelines();
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
            geometryPipeline->GetPipeline(),
            geometryPipeline->GetPipelineLayout()
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
                    ShaderCache::descriptorSets.lightsDescriptorSets[imageIndex],
                    ShaderCache::descriptorSets.irradianceDescriptorSet,
                };

                if (scene.reflectionProbe)
                {
                    geometryDrawParams.descriptorSets.push_back(scene.reflectionProbe->descriptorSet);
                }

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
            skyBoxPipeline->GetPipeline(),
            skyBoxPipeline->GetPipelineLayout()
        };

        skyboxDrawParams.descriptorSets = {
            scene.skybox->descriptorSet,
            ShaderCache::descriptorSets.transformDescriptorSet
        };

        device->DrawMeshlet(skyboxDrawParams);
    }

    void RenderPass::LoadPipelines()
    {
        LOG_TRACE("Building skybox pipeline");
        PipelineConfig skyboxPipelineConfig; {
            skyboxPipelineConfig.vertexShaderPath = "shader/spv/skybox.vert.spv";
            skyboxPipelineConfig.fragmentShaderPath = "shader/spv/skybox.frag.spv";

            skyboxPipelineConfig.cullMode = PipelineCullMode::Front;
            skyboxPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            skyboxPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

            skyboxPipelineConfig.descriptorSetLayouts = {
                ShaderCache::descriptorSetLayouts.cubemapDescriptorSetLayout,
                ShaderCache::descriptorSetLayouts.transformDescriptorSetLayout,
            };

            skyboxPipelineConfig.colorAttachments = {
                ImageFormat::RGBA8_UNORM,
            };

            skyboxPipelineConfig.disableBlending = true;
            skyboxPipelineConfig.enableDepthTest = false;
            skyboxPipelineConfig.depthAttachment = ImageFormat::DEPTH24_STENCIL8;

            skyboxPipelineConfig.renderPass = renderPass;
        }
        skyBoxPipeline = VulkanPipeline::Builder().Build(device, skyboxPipelineConfig);

        LOG_TRACE("Building geometry pipeline");
        PipelineConfig geometryPipelineConfig; {
            geometryPipelineConfig.vertexShaderPath = "shader/spv/base-pass.vert.spv";
            geometryPipelineConfig.fragmentShaderPath = "shader/spv/lighting-pass.frag.spv";

            geometryPipelineConfig.cullMode = PipelineCullMode::Back;
            geometryPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            geometryPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

            geometryPipelineConfig.descriptorSetLayouts = {
                ShaderCache::descriptorSetLayouts.materialDescriptorSetLayout,
                ShaderCache::descriptorSetLayouts.transformDescriptorSetLayout,
                ShaderCache::descriptorSetLayouts.lightsDescriptorSetLayout,
                ShaderCache::descriptorSetLayouts.irradianceDescriptorSetLayout,
                ShaderCache::descriptorSetLayouts.reflectionDescriptorSetLayout,
            };

            geometryPipelineConfig.colorAttachments = {
                ImageFormat::RGBA8_UNORM,
            };

            geometryPipelineConfig.disableBlending = true;
            geometryPipelineConfig.enableDepthTest = true;
            geometryPipelineConfig.depthAttachment = ImageFormat::DEPTH24_STENCIL8;

            geometryPipelineConfig.renderPass = renderPass;

            geometryPipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            geometryPipelineConfig.pushConstantData.offset = 0;
            geometryPipelineConfig.pushConstantData.size = sizeof(SimplePushConstantData);
        }
        geometryPipeline = VulkanPipeline::Builder().Build(device, geometryPipelineConfig);
    }
}
