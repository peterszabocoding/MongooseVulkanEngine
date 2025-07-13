#include "renderer/vulkan/pass/lighting/prefilter_map_pass.h"

#include <renderer/shader_cache.h>
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

    PrefilterMapPass::PrefilterMapPass(VulkanDevice* vulkanDevice, const VkExtent2D& _resolution)
        : VulkanPass(vulkanDevice, _resolution)
    {
        cubeMesh = ResourceManager::LoadMesh(device, "resources/models/cube.obj");
        LoadPipeline();
    }

    void PrefilterMapPass::Render(VkCommandBuffer commandBuffer, Camera* camera, Ref<VulkanFramebuffer> writeBuffer,
        Ref<VulkanFramebuffer> readBuffer)
    {
        device->SetViewportAndScissor(passResolution, commandBuffer);
        GetRenderPass()->Begin(commandBuffer, writeBuffer, passResolution);

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
        pushConstantData.resolution = cubemapResolution;

        drawCommandParams.pushConstantParams = {
            &pushConstantData,
            sizeof(PrefilterData),
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        };

        device->DrawMeshlet(drawCommandParams);

        GetRenderPass()->End(commandBuffer);
    }

    void PrefilterMapPass::Resize(VkExtent2D _resolution)
    {
        VulkanPass::Resize(_resolution);
    }

    void PrefilterMapPass::SetFaceIndex(uint32_t index)
    {
        faceIndex = index;
    }
    void PrefilterMapPass::SetRoughness(float _roughness)
    {
        roughness = _roughness;
    }

    void PrefilterMapPass::SetPassResolution(VkExtent2D _passResolution)
    {
        passResolution = _passResolution;
    }

    void PrefilterMapPass::SetCubemapResolution(float _cubemapResolution)
    {
        cubemapResolution = _cubemapResolution;
    }

    void PrefilterMapPass::LoadPipeline()
    {
        VulkanRenderPass::RenderPassConfig config;
        config.AddColorAttachment({.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT});

        renderPassHandle = device->CreateRenderPass(config);

        PipelineConfig iblPrefilterPipelineConfig;
        iblPrefilterPipelineConfig.vertexShaderPath = "cubemap.vert";
        iblPrefilterPipelineConfig.fragmentShaderPath = "prefilter.frag";

        iblPrefilterPipelineConfig.cullMode = PipelineCullMode::Back;
        iblPrefilterPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
        iblPrefilterPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

        iblPrefilterPipelineConfig.descriptorSetLayouts = {
            ShaderCache::descriptorSetLayouts.cubemapDescriptorSetLayout,
        };

        iblPrefilterPipelineConfig.colorAttachments = {
            ImageFormat::RGBA16_SFLOAT,
        };

        iblPrefilterPipelineConfig.disableBlending = true;
        iblPrefilterPipelineConfig.enableDepthTest = false;

        iblPrefilterPipelineConfig.pushConstantData = {
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(PrefilterData)
        };

        iblPrefilterPipelineConfig.renderPass = GetRenderPass()->Get();

        prefilterMapPipeline = VulkanPipeline::Builder().Build(device, iblPrefilterPipelineConfig);
    }
}
