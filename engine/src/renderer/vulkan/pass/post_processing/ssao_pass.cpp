#include "renderer/vulkan/pass/post_processing/ssao_pass.h"

#include <random>

#include "util/log.h"
#include "renderer/shader_cache.h"
#include "renderer/vulkan/vulkan_descriptor_writer.h"
#include "renderer/vulkan/vulkan_mesh.h"

namespace MongooseVK
{
    SSAOPass::SSAOPass(VulkanDevice* _device, VkExtent2D _resolution): VulkanPass(_device, _resolution)
    {
        VulkanRenderPass::RenderPassConfig config;
        config.AddColorAttachment({.imageFormat = VK_FORMAT_R8_UNORM});

        renderPassHandle = device->CreateRenderPass(config);

        screenRect = CreateScope<VulkanMeshlet>(device, Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);
        LoadPipeline();

        GenerateKernel();
        GenerateNoiseData();

        CreateFramebuffer();
        InitDescriptorSet();
        BuildOutputDescriptorSet();
    }

    SSAOPass::~SSAOPass()
    {
        device->DestroyRenderPass(renderPassHandle);
        vkFreeDescriptorSets(device->GetDevice(), device->GetShaderDescriptorPool().GetDescriptorPool(), 1, &ssaoDescriptorSet);;
    }

    void SSAOPass::Render(VkCommandBuffer commandBuffer, Camera& camera, Ref<VulkanFramebuffer> writeBuffer,
                          Ref<VulkanFramebuffer> readBuffer)
    {
        device->SetViewportAndScissor(framebuffer->GetExtent(), commandBuffer);
        VulkanRenderPass* renderPass = device->renderPassPool.Get(renderPassHandle.handle);

        renderPass->Begin(commandBuffer, framebuffer, framebuffer->GetExtent());

        DrawCommandParams drawParams{};
        drawParams.commandBuffer = commandBuffer;
        drawParams.meshlet = screenRect.get();

        drawParams.pipelineParams = {
            ssaoPipeline->GetPipeline(),
            ssaoPipeline->GetPipelineLayout()
        };

        drawParams.descriptorSets = {
            ShaderCache::descriptorSets.gbufferDescriptorSet,
            ssaoDescriptorSet,
            ShaderCache::descriptorSets.transformDescriptorSet,
        };

        ssaoParams.resolution = glm::vec2(resolution.width, resolution.height);
        drawParams.pushConstantParams = {
            &ssaoParams,
            sizeof(SSAOParams)
        };

        device->DrawMeshlet(drawParams);
        renderPass->End(commandBuffer);
    }

    VulkanRenderPass* SSAOPass::GetRenderPass()
    {
        return device->renderPassPool.Get(renderPassHandle.handle);
    }

    void SSAOPass::Resize(VkExtent2D _resolution)
    {
        VulkanPass::Resize(_resolution);
        CreateFramebuffer();
        BuildOutputDescriptorSet();
    }

    void SSAOPass::LoadPipeline()
    {
        VulkanRenderPass* renderPass = device->renderPassPool.Get(renderPassHandle.handle);

        LOG_TRACE("Building SSAO pipeline");
        PipelineConfig pipelineConfig;
        pipelineConfig.vertexShaderPath = "quad.vert";
        pipelineConfig.fragmentShaderPath = "post_processing_ssao.frag";

        pipelineConfig.cullMode = PipelineCullMode::Front;
        pipelineConfig.polygonMode = PipelinePolygonMode::Fill;
        pipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

        pipelineConfig.descriptorSetLayouts = {
            ShaderCache::descriptorSetLayouts.gBufferDescriptorSetLayout,
            ShaderCache::descriptorSetLayouts.ssaoDescriptorSetLayout,
            ShaderCache::descriptorSetLayouts.transformDescriptorSetLayout,
        };

        pipelineConfig.colorAttachments = {
            ImageFormat::R8_UNORM,
        };

        pipelineConfig.disableBlending = true;
        pipelineConfig.enableDepthTest = false;

        pipelineConfig.renderPass = renderPass->renderPass;

        pipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineConfig.pushConstantData.offset = 0;
        pipelineConfig.pushConstantData.size = sizeof(SSAOParams);

        ssaoPipeline = VulkanPipeline::Builder().Build(device, pipelineConfig);
    }

    void SSAOPass::InitDescriptorSet()
    {
        ssaoBuffer = CreateRef<VulkanBuffer>(
            device,
            sizeof(SSAOBuffer),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU);


        memcpy(ssaoBuffer->GetData(), &buffer, sizeof(SSAOBuffer));

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = ssaoBuffer->GetBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(SSAOBuffer);

        VkDescriptorImageInfo ssaoNoiseTextureInfo{};
        ssaoNoiseTextureInfo.sampler = ssaoNoiseTexture->GetSampler();
        ssaoNoiseTextureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        ssaoNoiseTextureInfo.imageView = ssaoNoiseTexture->GetImageView();

        VulkanDescriptorWriter(*ShaderCache::descriptorSetLayouts.ssaoDescriptorSetLayout, device->GetShaderDescriptorPool())
                .WriteBuffer(0, &bufferInfo)
                .WriteImage(1, &ssaoNoiseTextureInfo)
                .Build(ssaoDescriptorSet);
    }

    void SSAOPass::GenerateNoiseData()
    {
        std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
        std::default_random_engine generator;

        std::vector<glm::vec4> ssaoNoiseData;
        ssaoNoiseData.resize(16);
        for (unsigned int i = 0; i < 16; i++)
        {
            ssaoNoiseData[i] = glm::vec4(randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator) * 2.0f - 1.0f, 0.0f, 1.0f);
        }

        ssaoNoiseTexture = VulkanTextureBuilder()
                .SetResolution(4, 4)
                .SetFormat(ImageFormat::RGBA32_SFLOAT)
                .AddUsage(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                .AddUsage(VK_IMAGE_USAGE_SAMPLED_BIT)
                .SetFilter(VK_FILTER_LINEAR, VK_FILTER_LINEAR)
                .SetTiling(VK_IMAGE_TILING_OPTIMAL)
                .AddAspectFlag(VK_IMAGE_ASPECT_COLOR_BIT)
                .SetData(ssaoNoiseData.data(), ssaoNoiseData.size() * 4 * 4)
                .Build(device);
    }

    void SSAOPass::GenerateKernel()
    {
        std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
        std::default_random_engine generator;

        for (unsigned int i = 0; i < 64; ++i)
        {
            glm::vec4 sample(
                randomFloats(generator) * 2.0 - 1.0,
                randomFloats(generator) * 2.0 - 1.0,
                randomFloats(generator),
                0.0
            );
            sample = normalize(sample);
            sample *= randomFloats(generator);

            float scale = static_cast<float>(i) / 64.0;
            scale = lerp(0.1f, 1.0f, scale * scale);
            sample *= scale;

            buffer.samples[i] = sample;
        }
    }

    void SSAOPass::BuildOutputDescriptorSet()
    {
        VkDescriptorImageInfo ssaoInfo{};
        ssaoInfo.sampler = framebuffer->GetAttachments()[0].sampler;
        ssaoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        ssaoInfo.imageView = framebuffer->GetAttachments()[0].imageView;

        auto descriptorSetLayout = ShaderCache::descriptorSetLayouts.postProcessingDescriptorSetLayout;
        auto writer = VulkanDescriptorWriter(*descriptorSetLayout, device->GetShaderDescriptorPool())
                .WriteImage(0, &ssaoInfo);

        if (ShaderCache::descriptorSets.postProcessingDescriptorSet)
            writer.Overwrite(ShaderCache::descriptorSets.postProcessingDescriptorSet);
        else
            writer.Build(ShaderCache::descriptorSets.postProcessingDescriptorSet);
    }

    void SSAOPass::CreateFramebuffer()
    {
        framebuffer = VulkanFramebuffer::Builder(device)
                .SetRenderpass(device->renderPassPool.Get(renderPassHandle.handle))
                .SetResolution(resolution.width * 0.5, resolution.height * 0.5)
                .AddAttachment(ImageFormat::R8_UNORM)
                .Build();
    }
}
