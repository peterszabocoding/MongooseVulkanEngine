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
            if (descriptorSetLayoutHandle != INVALID_DESCRIPTOR_SET_LAYOUT_HANDLE)
            {
                device->DestroyDescriptorSetLayout(descriptorSetLayoutHandle);
                descriptorSetLayoutHandle = INVALID_DESCRIPTOR_SET_LAYOUT_HANDLE;
            }

            if (descriptorSet != VK_NULL_HANDLE)
            {
                vkFreeDescriptorSets(device->GetDevice(), device->GetShaderDescriptorPool().GetDescriptorPool(), 1, &descriptorSet);
                descriptorSet = VK_NULL_HANDLE;
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
                auto type = DescriptorSetBindingType::Unknown;
                switch (inputs[i]->type)
                {
                    case ResourceUsage::Type::Buffer:
                        type = DescriptorSetBindingType::UniformBuffer;
                        break;
                    case ResourceUsage::Type::Texture:
                        type = DescriptorSetBindingType::TextureSampler;
                        break;
                    default:
                        ASSERT(false, "Invalid resource type");
                }
                descriptorSetLayoutBuilder.AddBinding({i, type, {ShaderStage::VertexShader, ShaderStage::FragmentShader}});
            }
            descriptorSetLayoutHandle = descriptorSetLayoutBuilder.Build();

            auto descriptorSetWriter = VulkanDescriptorWriter(*device->GetDescriptorSetLayout(descriptorSetLayoutHandle),
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

                    VkImageLayout layout = IsDepthFormat(format)
                                               ? format == ImageFormat::DEPTH32
                                                     ? VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL
                                                     : VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
                                               : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                    VkDescriptorImageInfo imageInfo{};
                    imageInfo.sampler = texture->GetSampler();
                    imageInfo.imageView = texture->GetImageView();
                    imageInfo.imageLayout = layout;

                    descriptorSetWriter.WriteImage(i, imageInfo);
                }
            }

            descriptorSetWriter.Build(descriptorSet);
        }

        void FrameGraphRenderPass::CreateFramebuffer()
        {
            FramebufferCreateInfo framebufferCreateInfo;
            framebufferCreateInfo.renderPassHandle = renderPassHandle;
            framebufferCreateInfo.resolution = resolution;

            for (const auto& resource: outputs | std::views::keys)
            {
                if (resource->type == ResourceUsage::Type::Texture)
                    framebufferCreateInfo.attachments.push_back({.textureHandle = resource->textureHandle});

                if (resource->type == ResourceUsage::Type::ImageView)
                    framebufferCreateInfo.attachments.push_back({.imageView = resource->imageView});
            }


            framebufferHandles.push_back(device->CreateFramebuffer(framebufferCreateInfo));
        }
    }
}
