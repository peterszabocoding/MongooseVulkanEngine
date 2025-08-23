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
        VulkanRenderPass::RenderPassConfig renderpassConfig{};

        for (const auto& output: outputs)
        {
            if (output.resource.type == FrameGraphResourceType::Buffer) continue;

            const ImageFormat format = output.resource.resourceInfo.texture.textureCreateInfo.format;
            if (!IsDepthFormat(format))
            {
                renderpassConfig.AddColorAttachment({
                    .imageFormat = format,
                    .loadOp = output.loadOp,
                    .storeOp = output.storeOp,
                    .initialLayout = ImageUtils::GetLayoutFromFormat(format),
                    .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                });
            } else
            {
                renderpassConfig.AddDepthAttachment({
                    .depthFormat = format,
                    .loadOp = output.loadOp,
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
        for (const auto& output: outputs)
        {
            if (output.resource.type == FrameGraphResourceType::Buffer) continue;

            ImageFormat format = output.resource.resourceInfo.texture.textureCreateInfo.format;
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

    FrameGraph::FrameGraph(VulkanDevice* _device): device{_device}
    {
        resourcePool.Init(128);
    }

    void FrameGraph::Init(const VkExtent2D _resolution)
    {
        resolution = _resolution;
    }

    void FrameGraph::PreRender(const VkCommandBuffer cmd, SceneGraph* scene)
    {
        for (const auto& renderPass: renderPasses | std::views::values)
        {
            renderPass->PreRender(cmd, scene);
        }
    }

    void FrameGraph::Render(const VkCommandBuffer cmd, SceneGraph* scene)
    {
        for (const auto& renderPass: renderPasses | std::views::values)
        {
            renderPass->Render(cmd, scene);
        }
    }

    void FrameGraph::Resize(const VkExtent2D newResolution)
    {
        resolution = newResolution;

        for (const auto& renderPass: renderPasses | std::views::values)
        {
            renderPass->Resize(resolution);
        }
    }

    FrameGraphNode* FrameGraph::CreateNode(FrameGraphNodeCreation nodeCreation)
    {
        FrameGraphNode* node = new FrameGraphNode();
        node->name = nodeCreation.name;
        node->enabled = nodeCreation.enabled;

        for (auto& [name, type, resourceInfo]: nodeCreation.inputs)
        {
            // First check if input with this name was created already
            FrameGraphResourceHandle handle;
            if (resourceHandles.contains(name))
            {
                handle = resourceHandles[name];
            } else
            {
                handle = CreateResource(name, type, resourceInfo);
                resourceHandles[name] = handle;
            }
            node->inputs.push_back(handle);
        }

        for (auto& [resourceCreate, loadOp, storeOp]: nodeCreation.outputs)
        {
            // First check if output with this name was created already
            FrameGraphResourceHandle handle;
            if (resourceHandles.contains(resourceCreate.name))
            {
                handle = resourceHandles[resourceCreate.name];
            } else
            {
                handle = CreateResource(resourceCreate.name, resourceCreate.type, resourceCreate.resourceInfo);
                resourceHandles[resourceCreate.name] = handle;
            }
            node->outputs.push_back(handle);
        }

        nodes[nodeCreation.name] = node;

        return node;
    }

    FrameGraphResourceHandle FrameGraph::CreateResource(const char* resourceName, FrameGraphResourceType type,
                                                        FrameGraphResourceCreateInfo& createInfo)
    {
        if (type == FrameGraphResourceType::Texture) return CreateTextureResource(resourceName, createInfo);
        if (type == FrameGraphResourceType::Buffer) return CreateBufferResource(resourceName, createInfo);

        return {INVALID_RESOURCE_HANDLE};
    }

    FrameGraphResourceHandle FrameGraph::CreateTextureResource(const char* resourceName, FrameGraphResourceCreateInfo& createInfo)
    {
        createInfo.texture.textureHandle = device->CreateTexture(createInfo.texture.textureCreateInfo);

        FrameGraphResource* graphResource = resourcePool.Obtain();
        graphResource->name = resourceName;
        graphResource->type = FrameGraphResourceType::Texture;
        graphResource->resourceInfo = createInfo;

        return {graphResource->index};
    }

    FrameGraphResourceHandle FrameGraph::CreateBufferResource(const char* resourceName, FrameGraphResourceCreateInfo& createInfo)
    {
        createInfo.buffer.allocatedBuffer = device->CreateBuffer(
            createInfo.buffer.size,
            createInfo.buffer.usageFlags,
            createInfo.buffer.memoryUsage
        );

        FrameGraphResource* graphResource = resourcePool.Obtain();
        graphResource->name = resourceName;
        graphResource->type = FrameGraphResourceType::Buffer;
        graphResource->resourceInfo = createInfo;

        return {graphResource->index};
    }

    void FrameGraph::DestroyResources()
    {
        resourcePool.ForEach([&](const FrameGraphResource* frameGraphResource) {
            if (frameGraphResource->type == FrameGraphResourceType::Texture)
                device->DestroyTexture(frameGraphResource->resourceInfo.texture.textureHandle);

            if (frameGraphResource->type == FrameGraphResourceType::Buffer)
                device->DestroyBuffer(frameGraphResource->resourceInfo.buffer.allocatedBuffer);
        });

        resourcePool.FreeAllResources();
    }
}
