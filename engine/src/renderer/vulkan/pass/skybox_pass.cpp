#include "renderer/vulkan/pass/skybox_pass.h"

#include <renderer/vulkan/vulkan_framebuffer.h>
#include <renderer/vulkan/vulkan_mesh.h>

#include "renderer/shader_cache.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "resource/resource_manager.h"
#include "util/log.h"

namespace MongooseVK
{
    SkyboxPass::SkyboxPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution): FrameGraphRenderPass(vulkanDevice, _resolution)
    {
        cubeMesh = ResourceManager::LoadMesh(device, "resources/models/cube.obj");
    }

    void SkyboxPass::Setup(FrameGraph* frameGraph)
    {
        FrameGraphResourceCreate outputCreation{};
        outputCreation.name = "hdr_image";
        outputCreation.type = FrameGraphResourceType::Texture;
        outputCreation.resourceInfo.texture.textureCreateInfo = {
            .width = resolution.width,
            .height = resolution.height,
            .format = ImageFormat::RGBA16_SFLOAT,
            .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        };

        frameGraph->CreateResource(outputCreation);

        frameGraph->WriteResource("depth_map", RenderPassOperation::LoadOp::Load, RenderPassOperation::StoreOp::Store);
    }

    void SkyboxPass::Render(VkCommandBuffer commandBuffer, Scene* scene)
    {
        const VulkanFramebuffer* framebuffer = device->GetFramebuffer(framebufferHandles[0]);

        device->SetViewportAndScissor(framebuffer->extent, commandBuffer);
        GetRenderPass()->Begin(commandBuffer, framebuffer->framebuffer, framebuffer->extent);

        DrawCommandParams drawCommandParams{};
        drawCommandParams.commandBuffer = commandBuffer;
        drawCommandParams.meshlet = &cubeMesh->GetMeshlets()[0];
        drawCommandParams.pipelineHandle = pipelineHandle;
        drawCommandParams.descriptorSets = {
            device->bindlessTextureDescriptorSet,
            device->materialDescriptorSet,
            passDescriptorSet
        };

        SkyboxPushConstantData pushConstantData;
        pushConstantData.skyboxTextureIndex = scene->skyboxTexture.handle;

        drawCommandParams.pushConstantParams = {
            &pushConstantData,
            sizeof(SkyboxPushConstantData)
        };

        device->DrawMeshlet(drawCommandParams);

        GetRenderPass()->End(commandBuffer);
    }

    void SkyboxPass::LoadPipeline(PipelineCreateInfo& pipelineCreate)
    {
        pipelineCreate.name = "SkyboxPass";
        pipelineCreate.vertexShaderPath = "skybox.vert";
        pipelineCreate.fragmentShaderPath = "skybox.frag";

        pipelineCreate.cullMode = PipelineCullMode::Front;
        pipelineCreate.enableDepthTest = false;
        pipelineCreate.descriptorSetLayouts = {
            device->bindlessTexturesDescriptorSetLayoutHandle,
            device->materialsDescriptorSetLayoutHandle,
            passDescriptorSetLayoutHandle
        };

        pipelineCreate.pushConstantData.shaderStageBits = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineCreate.pushConstantData.size = sizeof(SkyboxPushConstantData);
    }
}
