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
            device->DestroyFramebuffer(framebufferHandle);
            framebufferHandle = INVALID_FRAMEBUFFER_HANDLE;

            // Render pass
            device->DestroyRenderPass(renderPassHandle);
            renderPassHandle = INVALID_RENDER_PASS_HANDLE;
            renderpassConfig = {};

            // Descriptors
            device->DestroyDescriptorSetLayout(descriptorSetLayoutHandle);
            descriptorSetLayoutHandle = INVALID_DESCRIPTOR_SET_LAYOUT_HANDLE;

            vkFreeDescriptorSets(device->GetDevice(), device->GetShaderDescriptorPool().GetDescriptorPool(), 1, &descriptorSet);
            descriptorSet = VK_NULL_HANDLE;

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
                descriptorSetLayoutBuilder.AddBinding({
                    i,
                    inputs[i].type == ResourceType::Buffer
                        ? DescriptorSetBindingType::UniformBuffer
                        : DescriptorSetBindingType::TextureSampler,
                    {ShaderStage::VertexShader, ShaderStage::FragmentShader}
                });
            }
            descriptorSetLayoutHandle = descriptorSetLayoutBuilder.Build();

            auto descriptorSetWriter = VulkanDescriptorWriter(*device->GetDescriptorSetLayout(descriptorSetLayoutHandle),
                                                              device->GetShaderDescriptorPool());
            for (uint32_t i = 0; i < inputs.size(); i++)
            {
                if (inputs[i].type == ResourceType::Buffer)
                {
                    VkDescriptorBufferInfo info{};
                    info.buffer = inputs[i].resourceInfo.buffer.allocatedBuffer.buffer;
                    info.offset = 0;
                    info.range = inputs[i].resourceInfo.buffer.size;

                    descriptorSetWriter.WriteBuffer(i, &info);
                }

                if (inputs[i].type == ResourceType::Texture || inputs[i].type == ResourceType::TextureCube)
                {
                    VulkanTexture* texture = device->GetTexture(inputs[i].resourceInfo.texture.textureHandle);

                    VkDescriptorImageInfo info{
                        texture->GetSampler(),
                        texture->GetImageView(),
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                    };

                    descriptorSetWriter.WriteImage(i, &info);
                }
            }

            descriptorSetWriter.Build(descriptorSet);
        }

        virtual void InitFramebuffer()
        {
            FramebufferCreateInfo framebufferCreateInfo = {
                .attachments = {},
                .renderPassHandle = renderPassHandle,
                .resolution = resolution,
            };

            for (const auto& output: outputs)
                framebufferCreateInfo.attachments.push_back({.textureHandle = output.resource.resourceInfo.texture.textureHandle});

            framebufferHandle = device->CreateFramebuffer(framebufferCreateInfo);
        }

    protected:
        VulkanDevice* device;
        VkExtent2D resolution;

        PipelineCreate pipelineConfig{};
        VulkanPipeline* pipeline = nullptr;
        PipelineHandle pipelineHandle = INVALID_PIPELINE_HANDLE;

        VulkanRenderPass::RenderPassConfig renderpassConfig{};
        RenderPassHandle renderPassHandle = INVALID_RENDER_PASS_HANDLE;

        DescriptorSetLayoutHandle descriptorSetLayoutHandle = INVALID_DESCRIPTOR_SET_LAYOUT_HANDLE;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

        FramebufferHandle framebufferHandle = INVALID_FRAMEBUFFER_HANDLE;

        std::vector<PassResource> inputs;
        std::vector<PassOutput> outputs;
    };
}
