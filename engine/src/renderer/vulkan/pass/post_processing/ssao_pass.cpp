#include "renderer/vulkan/pass/post_processing/ssao_pass.h"

#include <random>
#include <renderer/vulkan/vulkan_framebuffer.h>

#include "util/log.h"
#include "renderer/shader_cache.h"
#include "renderer/vulkan/vulkan_descriptor_writer.h"
#include "renderer/vulkan/vulkan_mesh.h"
#include "renderer/vulkan/vulkan_texture.h"

namespace MongooseVK
{
    constexpr float RESOLUTION_SCALE = 0.5f;

    SSAOPass::SSAOPass(VulkanDevice* _device, VkExtent2D _resolution): FrameGraphRenderPass(_device, VkExtent2D{0, 0})
    {
        screenRect = CreateScope<VulkanMesh>(device);
        screenRect->AddMeshlet(Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);

        resolution = {
            static_cast<uint32_t>(_resolution.width * RESOLUTION_SCALE),
            static_cast<uint32_t>(_resolution.height * RESOLUTION_SCALE)
        };

        GenerateKernel();
        GenerateNoiseData();
    }

    SSAOPass::~SSAOPass()
    {
        vkFreeDescriptorSets(device->GetDevice(), device->GetShaderDescriptorPool().GetDescriptorPool(), 1, &ssaoDescriptorSet);
    }

    void SSAOPass::CreateDescriptors()
    {
        ssaoDescriptorSetLayout = VulkanDescriptorSetLayoutBuilder(device)
                                  .AddBinding({0, DescriptorSetBindingType::UniformBuffer, {ShaderStage::FragmentShader}})
                                  .AddBinding({1, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                                  .Build();
        InitDescriptorSet();
        FrameGraphRenderPass::CreateDescriptors();
    }


    void SSAOPass::Render(VkCommandBuffer commandBuffer, SceneGraph* scene)
    {
        VulkanFramebuffer* framebuffer = device->GetFramebuffer(framebufferHandles[0]);

        device->SetViewportAndScissor(framebuffer->extent, commandBuffer);
        VulkanRenderPass* renderPass = device->renderPassPool.Get(renderPassHandle.handle);

        renderPass->Begin(commandBuffer, framebuffer->framebuffer, framebuffer->extent);

        DrawCommandParams drawParams{};
        drawParams.commandBuffer = commandBuffer;
        drawParams.meshlet = &screenRect->GetMeshlets()[0];
        drawParams.pipelineHandle = pipelineHandle;
        drawParams.descriptorSets = {
            passDescriptorSet,
            ssaoDescriptorSet,
        };

        ssaoParams.resolution = glm::vec2(resolution.width, resolution.height);
        drawParams.pushConstantParams = {
            &ssaoParams,
            sizeof(SSAOParams)
        };

        device->DrawMeshlet(drawParams);
        renderPass->End(commandBuffer);
    }

    void SSAOPass::Resize(VkExtent2D _resolution)
    {
        FrameGraphRenderPass::Resize(VkExtent2D{
            static_cast<uint32_t>(_resolution.width * RESOLUTION_SCALE),
            static_cast<uint32_t>(_resolution.height * RESOLUTION_SCALE)
        });
    }

    void SSAOPass::LoadPipeline(PipelineCreateInfo& pipelineCreate)
    {
        pipelineCreate.name = "SSAOPass";
        pipelineCreate.vertexShaderPath = "quad.vert";
        pipelineCreate.fragmentShaderPath = "post_processing_ssao.frag";

        pipelineCreate.cullMode = PipelineCullMode::Front;

        pipelineCreate.descriptorSetLayouts = {
            passDescriptorSetLayoutHandle,
            ssaoDescriptorSetLayout,
        };

        pipelineCreate.enableDepthTest = false;

        pipelineCreate.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineCreate.pushConstantData.size = sizeof(SSAOParams);
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

        VulkanTexture* ssaoNoiseTexture = device->texturePool.Get(ssaoNoiseTextureHandle.handle);

        VkDescriptorImageInfo ssaoNoiseTextureInfo{};
        ssaoNoiseTextureInfo.sampler = ssaoNoiseTexture->GetSampler();
        ssaoNoiseTextureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        ssaoNoiseTextureInfo.imageView = ssaoNoiseTexture->GetImageView();

        auto descriptorSetLayout = device->GetDescriptorSetLayout(ssaoDescriptorSetLayout);
        VulkanDescriptorWriter(*descriptorSetLayout, device->GetShaderDescriptorPool())
                .WriteBuffer(0, bufferInfo)
                .WriteImage(1, ssaoNoiseTextureInfo)
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

        ImageResource imageResource;
        imageResource.width = 4;
        imageResource.height = 4;
        imageResource.data = ssaoNoiseData.data();
        imageResource.size = ssaoNoiseData.size() * 4 * 4;
        imageResource.format = ImageFormat::RGBA32_SFLOAT;

        ssaoNoiseTextureHandle = device->CreateTexture({
            .width = imageResource.width,
            .height = imageResource.height,
            .format = imageResource.format,
            .data = imageResource.data,
            .size = imageResource.size,
        });
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
