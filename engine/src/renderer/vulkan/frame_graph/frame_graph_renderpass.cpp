#include "renderer/frame_graph/frame_graph_renderpass.h"

#include <ranges>
#include <renderer/vulkan/vulkan_descriptor_writer.h>
#include <renderer/vulkan/vulkan_image.h>
#include <renderer/vulkan/vulkan_texture.h>
#include <resource/resource.h>

namespace MongooseVK
{
    namespace FrameGraph
    {
        void FrameGraphRenderPass::Init()
        {
            CreateRenderPass();
            CreateDescriptors();
            CreateFramebuffer();
            CreatePipeline();
        }

        void FrameGraphRenderPass::Reset()
        {
            // Pipeline
            if (pipelineHandle != INVALID_PIPELINE_HANDLE)
            {
                device->DestroyPipeline(pipelineHandle);
                pipelineHandle = INVALID_PIPELINE_HANDLE;
            }

            // Framebuffer
            if (framebufferHandles.size() > 0)
            {
                for (const auto handle: framebufferHandles)
                    device->DestroyFramebuffer(handle);

                framebufferHandles.clear();
            }

            // Render pass
            if (renderPassHandle != INVALID_RENDER_PASS_HANDLE)
            {
                device->DestroyRenderPass(renderPassHandle);
                renderPassHandle = INVALID_RENDER_PASS_HANDLE;
            }

            // Descriptors
            if (passDescriptorSetLayoutHandle != INVALID_DESCRIPTOR_SET_LAYOUT_HANDLE)
            {
                device->DestroyDescriptorSetLayout(passDescriptorSetLayoutHandle);
                passDescriptorSetLayoutHandle = INVALID_DESCRIPTOR_SET_LAYOUT_HANDLE;
            }

            if (passDescriptorSet != VK_NULL_HANDLE)
            {
                vkFreeDescriptorSets(device->GetDevice(), device->GetShaderDescriptorPool().GetDescriptorPool(), 1, &passDescriptorSet);
                passDescriptorSet = VK_NULL_HANDLE;
            }

            inputs.clear();
            outputs.clear();
        }

        void FrameGraphRenderPass::Resize(const VkExtent2D _resolution)
        {
            resolution = _resolution;

            for (const auto handle: framebufferHandles)
                device->DestroyFramebuffer(handle);

            framebufferHandles.clear();

            CreateFramebuffer();
        }

        VulkanRenderPass* FrameGraphRenderPass::GetRenderPass() const
        {
            return device->renderPassPool.Get(renderPassHandle.handle);
        }

        void FrameGraphRenderPass::AddOutput(FrameGraphResource* output, ResourceUsage usage)
        {
            outputs.push_back({output, usage});
        }


        void FrameGraphRenderPass::AddInput(FrameGraphResource* input)
        {
            inputs.push_back(input);
        }

        void FrameGraphRenderPass::CreateRenderPass()
        {
            VulkanRenderPass::RenderPassConfig renderpassConfig{};

            for (const auto& [resource, usage]: outputs)
            {
                if (resource->type == ResourceUsage::Type::Buffer) continue;

                const ImageFormat format = resource->textureInfo.format;
                if (!IsDepthFormat(format))
                {
                    renderpassConfig.AddColorAttachment({
                        .imageFormat = format,
                        .loadOp = usage.access == ResourceUsage::Access::Write
                                      ? RenderPassOperation::LoadOp::Clear
                                      : RenderPassOperation::LoadOp::Load,
                        .storeOp = RenderPassOperation::StoreOp::Store,
                        .initialLayout = ImageUtils::GetLayoutFromFormat(format),
                        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    });
                } else
                {
                    renderpassConfig.AddDepthAttachment({
                        .depthFormat = format,
                        .loadOp = usage.access == ResourceUsage::Access::Write
                                      ? RenderPassOperation::LoadOp::Clear
                                      : RenderPassOperation::LoadOp::Load,
                    });
                }
            }

            renderPassHandle = device->CreateRenderPass(renderpassConfig);
        }

        void FrameGraphRenderPass::CreatePipeline()
        {
            PipelineCreateInfo pipelineCreate{};
            LoadPipeline(pipelineCreate);

            if (pipelineCreate.name == "" || pipelineCreate.vertexShaderPath == "" || pipelineCreate.fragmentShaderPath == "") return;

            LOG_TRACE(pipelineCreate.name);
            for (const auto& resource: outputs | std::views::keys)
            {
                ImageFormat format = resource->textureInfo.format;
                if (!IsDepthFormat(format))
                    pipelineCreate.colorAttachments.push_back(format);
                else
                    pipelineCreate.depthAttachment = format;
            }

            pipelineCreate.renderPass = GetRenderPass()->Get();
            pipelineHandle = VulkanPipelineBuilder().Build(device, pipelineCreate);
        }

        void FrameGraphRenderPass::CreateDescriptors()
        {
            auto descriptorSetLayoutBuilder = VulkanDescriptorSetLayoutBuilder(device);

            for (uint32_t i = 0; i < inputs.size(); i++)
            {
                const DescriptorSetBindingType type = inputs[i]->type == ResourceUsage::Type::Buffer
                                                          ? DescriptorSetBindingType::UniformBuffer
                                                          : DescriptorSetBindingType::TextureSampler;
                descriptorSetLayoutBuilder.AddBinding({i, type, {ShaderStage::VertexShader, ShaderStage::FragmentShader}});
            }
            passDescriptorSetLayoutHandle = descriptorSetLayoutBuilder.Build();

            auto descriptorSetWriter = VulkanDescriptorWriter(*device->GetDescriptorSetLayout(passDescriptorSetLayoutHandle),
                                                              device->GetShaderDescriptorPool());
            for (uint32_t i = 0; i < inputs.size(); i++)
            {
                if (inputs[i]->type == ResourceUsage::Type::Buffer)
                {
                    VkDescriptorBufferInfo bufferInfo{};
                    bufferInfo.buffer = inputs[i]->allocatedBuffer.buffer;
                    bufferInfo.offset = 0;
                    bufferInfo.range = inputs[i]->allocatedBuffer.info.size;

                    descriptorSetWriter.WriteBuffer(i, bufferInfo);
                }

                if (inputs[i]->type == ResourceUsage::Type::Texture)
                {
                    const VulkanTexture* texture = device->GetTexture(inputs[i]->textureHandle);
                    const ImageFormat format = texture->createInfo.format;

                    VkDescriptorImageInfo imageInfo{};
                    imageInfo.sampler = texture->GetSampler();
                    imageInfo.imageView = texture->GetImageView();
                    imageInfo.imageLayout = IsDepthFormat(format)
                                                ? format == ImageFormat::DEPTH32
                                                      ? VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL
                                                      : VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
                                                : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                    descriptorSetWriter.WriteImage(i, imageInfo);
                }
            }

            descriptorSetWriter.Build(passDescriptorSet);
        }

        void FrameGraphRenderPass::CreateFramebuffer()
        {
            FramebufferCreateInfo framebufferCreateInfo = {
                .attachments = {},
                .renderPassHandle = renderPassHandle,
                .resolution = resolution,
            };

            for (const auto& resource: outputs | std::views::keys)
            {
                const TextureHandle outputTextureHandle = resource->textureHandle;
                framebufferCreateInfo.attachments.push_back({.textureHandle = outputTextureHandle});
            }

            framebufferHandles.push_back(device->CreateFramebuffer(framebufferCreateInfo));
        }
    }
}
