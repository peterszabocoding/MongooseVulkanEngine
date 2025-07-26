#pragma once

#include <renderer/vulkan/vulkan_descriptor_writer.h>

#include "../vulkan_device.h"
#include "../vulkan_framebuffer.h"
#include "renderer/camera.h"
#include "renderer/vulkan/vulkan_texture.h"

namespace MongooseVK
{
    enum class ResourceType: int8_t {
        Invalid = -1,
        Texture = 0,
        TextureCube,
        Buffer,
    };

    struct RenderPassResourceInfo {
        union {
            struct {
                uint64_t size;
                VkBufferUsageFlags usageFlags;
                VmaMemoryUsage memoryUsage;
                AllocatedBuffer allocatedBuffer;
            } buffer;

            struct {
                TextureHandle textureHandle;
                TextureCreateInfo textureCreateInfo;
            } texture;
        };
    };

    struct PassResource {
        std::string name;
        ResourceType type;
        RenderPassResourceInfo resourceInfo{};
    };

    struct PassOutput {
        PassResource resource;
        RenderPassOperation::LoadOp loadOp;
        RenderPassOperation::StoreOp storeOp;
    };

    struct RenderPass {
        std::string name;

        RenderPassHandle renderPassHandle;
        FramebufferHandle framebufferHandle;
        PipelineHandle pipelineHandle;

        std::vector<PassResource> inputs;
        std::vector<PassResource> outputs;
    };

    class VulkanPass {
    public:
        struct Config {
            VkExtent2D imageExtent;
        };

    public:
        VulkanPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution): device(vulkanDevice), resolution(_resolution) {}

        virtual ~VulkanPass()
        {
            VulkanPass::Reset();
        }

        virtual void Init()
        {
            InitRenderPass();
            InitDescriptors();
            InitFramebuffer();
            LoadPipeline();
        }

        virtual void Reset()
        {
            // Pipeline
            device->DestroyPipeline(pipelineHandle);
            pipeline = nullptr;
            pipelineHandle = INVALID_PIPELINE_HANDLE;
            pipelineConfig = {};

            // Framebuffer
            for (const auto handle: framebufferHandles)
                device->DestroyFramebuffer(handle);

            framebufferHandles.clear();

            // Render pass
            device->DestroyRenderPass(renderPassHandle);
            renderPassHandle = INVALID_RENDER_PASS_HANDLE;
            renderpassConfig = {};

            // Descriptors
            device->DestroyDescriptorSetLayout(passDescriptorSetLayoutHandle);
            passDescriptorSetLayoutHandle = INVALID_DESCRIPTOR_SET_LAYOUT_HANDLE;

            vkFreeDescriptorSets(device->GetDevice(), device->GetShaderDescriptorPool().GetDescriptorPool(), 1, &passDescriptorSet);
            passDescriptorSet = VK_NULL_HANDLE;

            inputs.clear();
            outputs.clear();
        }

        virtual void Render(VkCommandBuffer commandBuffer, Camera* camera, FramebufferHandle writeBufferHandle) = 0;

        virtual void Resize(VkExtent2D _resolution)
        {
            resolution = _resolution;
        }

        virtual void OnResolutionChanged(const uint32_t width, const uint32_t height) {}

        virtual VulkanRenderPass* GetRenderPass() const
        {
            return device->renderPassPool.Get(renderPassHandle.handle);
        }

        virtual RenderPassHandle GetRenderPassHandle() const
        {
            return renderPassHandle;
        }

        void AddOutput(const PassOutput& output)
        {
            outputs.push_back(output);
        }

        void AddInput(const PassResource& input)
        {
            inputs.push_back(input);
        }

    protected:
        virtual void LoadPipeline() = 0;

        virtual void InitRenderPass()
        {
            for (const auto& output: outputs)
            {
                if (output.resource.type != ResourceType::Texture) continue;

                ImageFormat format = output.resource.resourceInfo.texture.textureCreateInfo.format;
                if (format != ImageFormat::DEPTH32 && format != ImageFormat::DEPTH24_STENCIL8)
                {
                    renderpassConfig.AddColorAttachment({
                        .imageFormat = format,
                        .loadOp = output.loadOp,
                        .storeOp = output.storeOp,
                        .initialLayout = output.resource.resourceInfo.texture.textureCreateInfo.imageInitialLayout,
                        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    });
                    pipelineConfig.colorAttachments.push_back(format);
                } else
                {
                    renderpassConfig.AddDepthAttachment({
                        .depthFormat = format,
                        .loadOp = output.loadOp,
                    });
                    pipelineConfig.depthAttachment = format;
                }
            }

            renderPassHandle = device->CreateRenderPass(renderpassConfig);
        }

        virtual void InitDescriptors()
        {
            auto descriptorSetLayoutBuilder = VulkanDescriptorSetLayoutBuilder(device);

            for (uint32_t i = 0; i < inputs.size(); i++)
            {
                const DescriptorSetBindingType type = inputs[i].type == ResourceType::Buffer
                                                          ? DescriptorSetBindingType::UniformBuffer
                                                          : DescriptorSetBindingType::TextureSampler;
                descriptorSetLayoutBuilder.AddBinding({i, type, {ShaderStage::VertexShader, ShaderStage::FragmentShader}});
            }
            passDescriptorSetLayoutHandle = descriptorSetLayoutBuilder.Build();

            auto descriptorSetWriter = VulkanDescriptorWriter(*device->GetDescriptorSetLayout(passDescriptorSetLayoutHandle),
                                                              device->GetShaderDescriptorPool());
            for (uint32_t i = 0; i < inputs.size(); i++)
            {
                if (inputs[i].type == ResourceType::Buffer)
                {
                    VkDescriptorBufferInfo bufferInfo{};
                    bufferInfo.buffer = inputs[i].resourceInfo.buffer.allocatedBuffer.buffer;
                    bufferInfo.offset = 0;
                    bufferInfo.range = inputs[i].resourceInfo.buffer.size;

                    descriptorSetWriter.WriteBuffer(i, bufferInfo);
                }

                if (inputs[i].type == ResourceType::Texture || inputs[i].type == ResourceType::TextureCube)
                {
                    const VulkanTexture* texture = device->GetTexture(inputs[i].resourceInfo.texture.textureHandle);

                    VkDescriptorImageInfo imageInfo{};
                    imageInfo.sampler = texture->GetSampler();
                    imageInfo.imageView = texture->GetImageView();
                    imageInfo.imageLayout = texture->createInfo.imageLayout;

                    descriptorSetWriter.WriteImage(i, imageInfo);
                }
            }

            descriptorSetWriter.Build(passDescriptorSet);
        }

        virtual void InitFramebuffer()
        {
            FramebufferCreateInfo framebufferCreateInfo = {
                .attachments = {},
                .renderPassHandle = renderPassHandle,
                .resolution = resolution,
            };

            for (const auto& output: outputs)
            {
                const TextureHandle outputTextureHandle = output.resource.resourceInfo.texture.textureHandle;
                framebufferCreateInfo.attachments.push_back({.textureHandle = outputTextureHandle});
            }

            framebufferHandles.push_back(device->CreateFramebuffer(framebufferCreateInfo));
        }

    protected:
        VulkanDevice* device;
        VkExtent2D resolution;

        PipelineCreate pipelineConfig{};
        VulkanPipeline* pipeline = nullptr;
        PipelineHandle pipelineHandle = INVALID_PIPELINE_HANDLE;

        VulkanRenderPass::RenderPassConfig renderpassConfig{};
        RenderPassHandle renderPassHandle = INVALID_RENDER_PASS_HANDLE;

        std::vector<FramebufferHandle> framebufferHandles;

        DescriptorSetLayoutHandle passDescriptorSetLayoutHandle = INVALID_DESCRIPTOR_SET_LAYOUT_HANDLE;
        VkDescriptorSet passDescriptorSet = VK_NULL_HANDLE;

        std::vector<PassResource> inputs;
        std::vector<PassOutput> outputs;
    };
}
