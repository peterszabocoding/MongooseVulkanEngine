#include "ssao_pass.h"

#include <random>

#include "renderer/shader_cache.h"
#include "renderer/vulkan/vulkan_descriptor_writer.h"
#include "util/log.h"

namespace Raytracing
{
    SSAOPass::SSAOPass(VulkanDevice* _device): VulkanPass(_device)
    {
        renderPass = VulkanRenderPass::Builder(device)
                .AddColorAttachment(VK_FORMAT_R8_UNORM, false)
                .Build();
        screenRect = CreateScope<VulkanMeshlet>(device, Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);
        LoadPipeline();

        GenerateKernel();
        GenerateNoiseData();
        InitDescriptorSet();
    }

    SSAOPass::~SSAOPass()
    {
        vkFreeDescriptorSets(device->GetDevice(), device->GetShaderDescriptorPool().GetDescriptorPool(), 1, &ssaoDescriptorSet);;
    }

    void SSAOPass::Update()
    {}

    void SSAOPass::Render(VkCommandBuffer commandBuffer, uint32_t imageIndex, Ref<VulkanFramebuffer> writeBuffer,
                          Ref<VulkanFramebuffer> readBuffer)
    {
        VkExtent2D extent{passWidth, passHeight};

        device->SetViewportAndScissor(extent, commandBuffer);
        renderPass->Begin(commandBuffer, writeBuffer, extent);

        DrawCommandParams drawParams{};
        drawParams.commandBuffer = commandBuffer;
        drawParams.meshlet = screenRect.get();

        drawParams.pipelineParams = {
            ssaoPipeline->GetPipeline(),
            ssaoPipeline->GetPipelineLayout()
        };

        drawParams.descriptorSets = {
            ShaderCache::descriptorSets.gbufferDescriptorSets[imageIndex],
            ssaoDescriptorSet,
            ShaderCache::descriptorSets.transformDescriptorSet,

        };

        ssaoParams.resolution = glm::vec2(passWidth, passHeight);
        drawParams.pushConstantParams = {
            &ssaoParams,
            sizeof(SSAOParams)
        };

        device->DrawMeshlet(drawParams);
        renderPass->End(commandBuffer);
    }

    void SSAOPass::LoadPipeline()
    {
        {
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

            pipelineConfig.renderPass = renderPass;

            pipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            pipelineConfig.pushConstantData.offset = 0;
            pipelineConfig.pushConstantData.size = sizeof(SSAOParams);

            ssaoPipeline = VulkanPipeline::Builder().Build(device, pipelineConfig);
        }
    }

    void SSAOPass::InitDescriptorSet()
    {
        ssaoBuffer = CreateRef<VulkanBuffer>(
            device,
            sizeof(SSAOBuffer),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU);


        memcpy(ssaoBuffer->GetMappedData(), &buffer, sizeof(SSAOBuffer));

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

        ssaoNoiseTexture = VulkanTexture::Builder()
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
}
