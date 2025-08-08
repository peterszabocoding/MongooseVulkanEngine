#include "renderer/vulkan/pass/lighting/irradiance_map_pass.h"

#include <renderer/shader_cache.h>
#include <renderer/vulkan/vulkan_framebuffer.h>
#include <renderer/vulkan/vulkan_mesh.h>
#include <renderer/vulkan/vulkan_texture.h>
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

    IrradianceMapPass::IrradianceMapPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution): FrameGraphRenderPass(
        vulkanDevice, _resolution)
    {
        cubeMesh = ResourceManager::LoadMesh(device, "resources/models/cube.obj");
    }

    void IrradianceMapPass::CreateFramebuffer()
    {
        TextureHandle outputHandle = outputs[0].resource.resourceInfo.texture.textureHandle;
        VulkanTexture* outputTexture = device->GetTexture(outputHandle);

        std::vector<FramebufferHandle> iblIrradianceFramebuffes;
        iblIrradianceFramebuffes.resize(6);

        for (size_t i = 0; i < 6; i++)
        {
            FramebufferCreateInfo createInfo{};

            createInfo.attachments.push_back({outputTexture->GetMipmapImageView(0, i)});
            createInfo.renderPassHandle = renderPassHandle;
            createInfo.resolution = {32, 32};

            framebufferHandles.push_back(device->CreateFramebuffer(createInfo));
        }
    }

    void IrradianceMapPass::Render(VkCommandBuffer commandBuffer, Scene* scene)
    {
        for (uint32_t faceIndex = 0; faceIndex < 6; faceIndex++)
        {
            VulkanFramebuffer* framebuffer = device->GetFramebuffer(framebufferHandles[faceIndex]);

            device->SetViewportAndScissor(framebuffer->extent, commandBuffer);
            GetRenderPass()->Begin(commandBuffer, framebuffer->framebuffer, framebuffer->extent);

            DrawCommandParams drawCommandParams{};
            drawCommandParams.commandBuffer = commandBuffer;
            drawCommandParams.meshlet = &cubeMesh->GetMeshlets()[0];
            drawCommandParams.pipelineHandle = pipelineHandle;
            drawCommandParams.descriptorSets = {passDescriptorSet};

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
    }

    void IrradianceMapPass::Resize(VkExtent2D _resolution)
    {
        FrameGraphRenderPass::Resize(_resolution);
    }

    void IrradianceMapPass::LoadPipeline(PipelineCreateInfo& pipelineCreate)
    {
        pipelineCreate.name = "IrradianceMapPass";
        pipelineCreate.vertexShaderPath = "cubemap.vert";
        pipelineCreate.fragmentShaderPath = "irradiance_convolution.frag";

        pipelineCreate.descriptorSetLayouts = {
            passDescriptorSetLayoutHandle
        };

        pipelineCreate.enableDepthTest = false;

        pipelineCreate.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineCreate.pushConstantData.size = sizeof(TransformPushConstantData);

        LOG_TRACE(pipelineCreate.name);
    }

    void IrradianceMapPass::SetFaceIndex(uint8_t _faceIndex)
    {
        faceIndex = _faceIndex;
    }
}
