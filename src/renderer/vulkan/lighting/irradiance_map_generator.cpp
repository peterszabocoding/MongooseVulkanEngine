#include "irradiance_map_generator.h"

#include <vector>

#include "renderer/shader_cache.h"
#include "renderer/vulkan/vulkan_cube_map_texture.h"
#include "renderer/vulkan/vulkan_descriptor_writer.h"
#include "renderer/vulkan/vulkan_framebuffer.h"
#include "resource/resource_manager.h"

namespace Raytracing
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

    IrradianceMapGenerator::IrradianceMapGenerator(VulkanDevice* _device): device(_device)
    {
        cubeMesh = ResourceManager::LoadMesh(device, "resources/models/cube.obj");
        renderPass = VulkanRenderPass::Builder(device)
                .AddColorAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, false)
                .Build();
        LoadPipeline();
    }

    Ref<VulkanCubeMapTexture> IrradianceMapGenerator::FromCubemapTexture(const Ref<VulkanCubeMapTexture>& cubemapTexture)
    {

        Ref<VulkanCubeMapTexture> irradianceMap = VulkanCubeMapTexture::Builder()
                .SetFormat(ImageFormat::RGBA16_SFLOAT)
                .SetResolution(32, 32)
                .Build(device);

        std::vector<Ref<VulkanFramebuffer>> iblIrradianceFramebuffes;
        iblIrradianceFramebuffes.resize(6);
        for (size_t i = 0; i < 6; i++)
        {
            iblIrradianceFramebuffes[i] = VulkanFramebuffer::Builder(device)
                    .SetRenderpass(renderPass)
                    .SetResolution(32, 32)
                    .AddAttachment(irradianceMap->GetFaceImageView(i, 0))
                    .Build();
        }

        VkDescriptorSet cubemapDescriptorSet;
        VkDescriptorImageInfo info{
            cubemapTexture->GetSampler(),
            cubemapTexture->GetImageView(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        VulkanDescriptorWriter(*ShaderCache::descriptorSetLayouts.cubemapDescriptorSetLayout, device->GetShaderDescriptorPool())
                .WriteImage(0, &info)
                .Build(cubemapDescriptorSet);

        device->ImmediateSubmit([&](const VkCommandBuffer commandBuffer) {
            for (size_t faceIndex = 0; faceIndex < 6; faceIndex++)
            {
                ComputeIrradianceMap(commandBuffer, faceIndex, iblIrradianceFramebuffes[faceIndex], cubemapDescriptorSet);
            }
        });

        vkFreeDescriptorSets(device->GetDevice(), device->GetShaderDescriptorPool().GetDescriptorPool(), 1, &cubemapDescriptorSet);

        return irradianceMap;
    }

    void IrradianceMapGenerator::ComputeIrradianceMap(const VkCommandBuffer commandBuffer,
                                                      const size_t faceIndex,
                                                      const Ref<VulkanFramebuffer>& framebuffer,
                                                      VkDescriptorSet cubemapDescriptorSet)
    {
        VkExtent2D extent = {framebuffer->GetWidth(), framebuffer->GetHeight()};

        device->SetViewportAndScissor(extent, commandBuffer);
        renderPass->Begin(commandBuffer, framebuffer, extent);

        DrawCommandParams drawCommandParams{};
        drawCommandParams.commandBuffer = commandBuffer;
        drawCommandParams.meshlet = &cubeMesh->GetMeshlets()[0];
        drawCommandParams.descriptorSets = {cubemapDescriptorSet};

        drawCommandParams.pipelineParams = {
            irradianceMapPipeline->GetPipeline(),
            irradianceMapPipeline->GetPipelineLayout()
        };

        TransformPushConstantData pushConstantData;
        pushConstantData.projection = m_CaptureProjection;
        pushConstantData.view = m_CaptureViews[faceIndex];

        drawCommandParams.pushConstantParams = {
            &pushConstantData,
            sizeof(TransformPushConstantData)
        };

        device->DrawMeshlet(drawCommandParams);

        renderPass->End(commandBuffer);
    }

    void IrradianceMapGenerator::LoadPipeline()
    {
        PipelineConfig iblIrradianceMapPipelineConfig; {
            iblIrradianceMapPipelineConfig.vertexShaderPath = "cubemap.vert";
            iblIrradianceMapPipelineConfig.fragmentShaderPath = "irradiance_convolution.frag";

            iblIrradianceMapPipelineConfig.cullMode = PipelineCullMode::Back;
            iblIrradianceMapPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            iblIrradianceMapPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

            iblIrradianceMapPipelineConfig.descriptorSetLayouts = {
                ShaderCache::descriptorSetLayouts.cubemapDescriptorSetLayout,
            };

            iblIrradianceMapPipelineConfig.colorAttachments = {
                ImageFormat::RGBA16_SFLOAT,
            };

            iblIrradianceMapPipelineConfig.disableBlending = true;
            iblIrradianceMapPipelineConfig.enableDepthTest = false;

            iblIrradianceMapPipelineConfig.renderPass = renderPass;

            iblIrradianceMapPipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            iblIrradianceMapPipelineConfig.pushConstantData.offset = 0;
            iblIrradianceMapPipelineConfig.pushConstantData.size = sizeof(TransformPushConstantData);
        }
        irradianceMapPipeline = VulkanPipeline::Builder().Build(device, iblIrradianceMapPipelineConfig);
    }
}
