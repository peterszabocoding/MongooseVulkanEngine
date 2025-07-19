#include "renderer/vulkan/pass/lighting/irradiance_map_pass.h"

#include <renderer/shader_cache.h>
#include <renderer/vulkan/vulkan_mesh.h>
#include <resource/resource_manager.h>

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

    IrradianceMapPass::IrradianceMapPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution): VulkanPass(vulkanDevice, _resolution)
    {
        cubeMesh = ResourceManager::LoadMesh(device, "resources/models/cube.obj");
        LoadPipeline();
    }

    void IrradianceMapPass::Render(VkCommandBuffer commandBuffer, Camera* camera, FramebufferHandle writeBuffer)
    {
        VulkanFramebuffer* framebuffer = device->GetFramebuffer(writeBuffer);

        device->SetViewportAndScissor(framebuffer->extent, commandBuffer);
        GetRenderPass()->Begin(commandBuffer, framebuffer->framebuffer, framebuffer->extent);

        DrawCommandParams drawCommandParams{};
        drawCommandParams.commandBuffer = commandBuffer;
        drawCommandParams.meshlet = &cubeMesh->GetMeshlets()[0];
        drawCommandParams.descriptorSets = {
            ShaderCache::descriptorSets.cubemapDescriptorSet
        };

        drawCommandParams.pipelineParams = {
            irradianceMapPipeline->pipeline,
            irradianceMapPipeline->pipelineLayout
        };

        TransformPushConstantData pushConstantData;
        pushConstantData.projection = m_CaptureProjection;
        pushConstantData.view = m_CaptureViews[faceIndex];

        drawCommandParams.pushConstantParams = {
            &pushConstantData,
            sizeof(TransformPushConstantData)
        };

        device->DrawMeshlet(drawCommandParams);

        GetRenderPass()->End(commandBuffer);
    }

    void IrradianceMapPass::Resize(VkExtent2D _resolution)
    {
        VulkanPass::Resize(_resolution);
    }

    void IrradianceMapPass::LoadPipeline()
    {
        VulkanRenderPass::RenderPassConfig config;
        config.AddColorAttachment({
            .imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT
        });

        renderPassHandle = device->CreateRenderPass(config);

        PipelineCreate iblIrradianceMapPipelineConfig;
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

        iblIrradianceMapPipelineConfig.renderPass = GetRenderPass()->Get();

        iblIrradianceMapPipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        iblIrradianceMapPipelineConfig.pushConstantData.offset = 0;
        iblIrradianceMapPipelineConfig.pushConstantData.size = sizeof(TransformPushConstantData);

        irradianceMapPipeline = VulkanPipelineBuilder().Build(device, iblIrradianceMapPipelineConfig);
    }

    void IrradianceMapPass::SetFaceIndex(uint8_t _faceIndex)
    {
        faceIndex = _faceIndex;
    }
}
