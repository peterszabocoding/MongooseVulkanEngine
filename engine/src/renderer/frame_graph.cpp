#include "renderer\frame_graph.h"

#include <ranges>
#include <renderer/vulkan/vulkan_descriptor_writer.h>
#include <renderer/vulkan/vulkan_renderer.h>
#include <renderer/vulkan/vulkan_texture.h>

namespace MongooseVK
{
    void FrameGraphRenderPass::Init()
    {
        CreateRenderPass();
        CreateDescriptors();
        CreateFramebuffer();
        LoadPipeline();
    }

    void FrameGraphRenderPass::Reset()
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

    void FrameGraphRenderPass::Resize(VkExtent2D _resolution)
    {
        resolution = _resolution;
    }

    VulkanRenderPass* FrameGraphRenderPass::GetRenderPass() const
    {
        return device->renderPassPool.Get(renderPassHandle.handle);
    }

    RenderPassHandle FrameGraphRenderPass::GetRenderPassHandle() const
    {
        return renderPassHandle;
    }

    void FrameGraphRenderPass::AddOutput(const FrameGraphNodeOutput& output)
    {
        outputs.push_back(output);
    }

    void FrameGraphRenderPass::AddInput(const FrameGraphResource& input)
    {
        inputs.push_back(input);
    }

    void FrameGraphRenderPass::CreateRenderPass()
    {
        for (const auto& output: outputs)
        {
            if (output.resource.type == FrameGraphResourceType::Buffer) continue;

            ImageFormat format = output.resource.resourceInfo.texture.textureCreateInfo.format;
            if (!IsDepthFormat(format))
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

    void FrameGraphRenderPass::CreateDescriptors()
    {
        auto descriptorSetLayoutBuilder = VulkanDescriptorSetLayoutBuilder(device);

        for (uint32_t i = 0; i < inputs.size(); i++)
        {
            const DescriptorSetBindingType type = inputs[i].type == FrameGraphResourceType::Buffer
                                                      ? DescriptorSetBindingType::UniformBuffer
                                                      : DescriptorSetBindingType::TextureSampler;
            descriptorSetLayoutBuilder.AddBinding({i, type, {ShaderStage::VertexShader, ShaderStage::FragmentShader}});
        }
        passDescriptorSetLayoutHandle = descriptorSetLayoutBuilder.Build();

        auto descriptorSetWriter = VulkanDescriptorWriter(*device->GetDescriptorSetLayout(passDescriptorSetLayoutHandle),
                                                          device->GetShaderDescriptorPool());
        for (uint32_t i = 0; i < inputs.size(); i++)
        {
            if (inputs[i].type == FrameGraphResourceType::Buffer)
            {
                VkDescriptorBufferInfo bufferInfo{};
                bufferInfo.buffer = inputs[i].resourceInfo.buffer.allocatedBuffer.buffer;
                bufferInfo.offset = 0;
                bufferInfo.range = inputs[i].resourceInfo.buffer.size;

                descriptorSetWriter.WriteBuffer(i, bufferInfo);
            }

            if (inputs[i].type == FrameGraphResourceType::Texture || inputs[i].type == FrameGraphResourceType::TextureCube)
            {
                const VulkanTexture* texture = device->GetTexture(inputs[i].resourceInfo.texture.textureHandle);
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

        for (const auto& output: outputs)
        {
            const TextureHandle outputTextureHandle = output.resource.resourceInfo.texture.textureHandle;
            framebufferCreateInfo.attachments.push_back({.textureHandle = outputTextureHandle});
        }

        framebufferHandles.push_back(device->CreateFramebuffer(framebufferCreateInfo));
    }

    FrameGraph::FrameGraph(VulkanDevice* _device): device{_device} {}

    void FrameGraph::Init(Scene* _scene, VkExtent2D _resolution)
    {
        scene = _scene;
        resolution = _resolution;
    }

    void FrameGraph::PreRender(VkCommandBuffer cmd)
    {
        for (const auto& renderPass: frameGraphRenderPasses | std::views::values)
        {
            renderPass->PreRender(cmd);
        }
    }

    void FrameGraph::Render(VkCommandBuffer cmd)
    {
        for (const auto& renderPass: frameGraphRenderPasses | std::views::values)
        {
            renderPass->Render(cmd);
        }
    }

    void FrameGraph::Resize(VkExtent2D newResolution)
    {
        resolution = newResolution;

        for (const auto& renderPass: frameGraphRenderPasses | std::views::values)
        {
            renderPass->Resize(resolution);
        }
    }

    void FrameGraph::AddRenderPass(const std::string& name, FrameGraphRenderPass* frameGraphRenderPass)
    {
        frameGraphRenderPasses[name] = frameGraphRenderPass;
    }

    void FrameGraph::AddResource(FrameGraphResource* frameGraphResouce)
    {
        frameGraphResources[frameGraphResouce->name] = frameGraphResouce;
    }
}
