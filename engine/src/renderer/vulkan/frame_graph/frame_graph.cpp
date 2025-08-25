#include "renderer/frame_graph/frame_graph.h"

#include <ranges>
#include <renderer/vulkan/vulkan_renderer.h>
#include <renderer/vulkan/vulkan_texture.h>
#include <renderer/vulkan/pass/gbufferPass.h>
#include <renderer/vulkan/pass/infinite_grid_pass.h>
#include <renderer/vulkan/pass/lighting_pass.h>
#include <renderer/vulkan/pass/shadow_map_pass.h>
#include <renderer/vulkan/pass/skybox_pass.h>
#include <renderer/vulkan/pass/ui_pass.h>
#include <renderer/vulkan/pass/post_processing/ssao_pass.h>
#include <renderer/vulkan/pass/post_processing/tone_mapping_pass.h>
#include <tiny_gltf/tiny_gltf.h>

namespace MongooseVK
{
    namespace FrameGraph
    {
        void PassBuilder::CreateTexture(const char* name, TextureCreateInfo& info)
        {
            frameGraph.CreateFrameGraphTextureResource(name, info);
        }

        void PassBuilder::CreateBuffer(const char* name, FrameGraphBufferCreateInfo& info)
        {
            frameGraph.CreateFrameGraphBufferResource(name, info);
        }

        void PassBuilder::Read(const char* name)
        {
            pass->readResources.push_back(name);
        }

        VkExtent2D PassBuilder::GetResolution() const
        {
            return frameGraph.resolution;
        }

        void PassBuilder::Write(const char* name)
        {
            pass->writeResources.push_back(name);
        }

        FrameGraph::FrameGraph(VulkanDevice* _device): device{_device}, resolution({0, 0})
        {
            resourcePool.Init(128);
        }

        void FrameGraph::Compile(const VkExtent2D _resolution)
        {
            resolution = _resolution;

            CreateFrameGraphOutputs();
            InitializeRenderPasses();
        }

        void FrameGraph::Execute(const VkCommandBuffer cmd, SceneGraph* scene)
        {
            for (const auto& renderPass: renderPassList)
                renderPass->Render(cmd, scene);

            for (const auto& pass: passes)
                pass->Execute(cmd, {});
        }

        void FrameGraph::Resize(const VkExtent2D newResolution)
        {
            Cleanup();
            Compile(newResolution);
        }

        void FrameGraph::Cleanup()
        {
            DestroyResources();

            for (const auto& renderPass: renderPassList)
                delete renderPass;

            renderPassList.clear();
            renderPasses.clear();
            renderPassResourceMap.clear();
            resourceHandles.clear();
        }

        void FrameGraph::AddExternalResource(const char* name, FrameGraphResource* resource)
        {
            externalResources[name] = resource;
        }

        FrameGraphResource* FrameGraph::GetResource(const char* name)
        {
            return renderPassResourceMap[name];
        }

        FrameGraphResourceHandle FrameGraph::CreateTextureResource(const char* resourceName, TextureCreateInfo& createInfo)
        {
            FrameGraphResource* graphResource = resourcePool.Obtain();
            graphResource->name = resourceName;
            graphResource->type = ResourceUsage::Type::Texture;
            graphResource->textureInfo = createInfo;
            graphResource->textureHandle = device->CreateTexture(createInfo);

            return {graphResource->index};
        }

        FrameGraphResourceHandle FrameGraph::CreateBufferResource(const char* resourceName, FrameGraphBufferCreateInfo& createInfo)
        {
            FrameGraphResource* graphResource = resourcePool.Obtain();
            graphResource->name = resourceName;
            graphResource->type = ResourceUsage::Type::Buffer;
            graphResource->allocatedBuffer = device->CreateBuffer(
                createInfo.size,
                createInfo.usageFlags,
                createInfo.memoryUsage
            );

            return {graphResource->index};
        }

        void FrameGraph::DestroyResources()
        {
            resourcePool.ForEach([&](const FrameGraphResource* frameGraphResource) {
                if (frameGraphResource->type == ResourceUsage::Type::Texture)
                    device->DestroyTexture(frameGraphResource->textureHandle);

                if (frameGraphResource->type == ResourceUsage::Type::Buffer)
                    device->DestroyBuffer(frameGraphResource->allocatedBuffer);
            });

            resourcePool.FreeAllResources();
        }

        void FrameGraph::InitializeRenderPasses()
        {
            AddRenderPass<ShadowMapPass>("ShadowMapPass");
            AddRenderPass<GBufferPass>("GBufferPass");
            AddRenderPass<SSAOPass>("SSAOPass");
            AddRenderPass<SkyboxPass>("SkyboxPass");
            AddRenderPass<LightingPass>("LightingPass");
            AddRenderPass<InfiniteGridPass>("InfiniteGridPass");
            AddRenderPass<ToneMappingPass>("ToneMappingPass");
            AddRenderPass<UiPass>("UiPass");

            for (const auto& renderPass: renderPasses | std::views::values)
                renderPass->Reset();

            // Skybox pass
            {
                renderPasses["SkyboxPass"]->AddInput(externalResources["camera_buffer"]);
                renderPasses["SkyboxPass"]->AddOutput(renderPassResourceMap["hdr_image"],
                                                      {
                                                          ResourceUsage::Access::Write,
                                                          ResourceUsage::Type::Texture,
                                                          ResourceUsage::Usage::ColorAttachment
                                                      });
                renderPasses["SkyboxPass"]->AddOutput(renderPassResourceMap["depth_map"],
                                                      {
                                                          ResourceUsage::Access::Write,
                                                          ResourceUsage::Type::Texture,
                                                          ResourceUsage::Usage::DepthStencil
                                                      });
            }

            // Grid pass
            {
                renderPasses["InfiniteGridPass"]->AddInput(externalResources["camera_buffer"]);

                renderPasses["InfiniteGridPass"]->AddOutput(renderPassResourceMap["hdr_image"],
                                                            {
                                                                ResourceUsage::Access::ReadWrite,
                                                                ResourceUsage::Type::Texture,
                                                                ResourceUsage::Usage::ColorAttachment
                                                            });

                renderPasses["InfiniteGridPass"]->AddOutput(renderPassResourceMap["depth_map"],
                                                            {
                                                                ResourceUsage::Access::ReadWrite,
                                                                ResourceUsage::Type::Texture,
                                                                ResourceUsage::Usage::DepthStencil
                                                            });
            }

            // GBuffer pass
            {
                renderPasses["GBufferPass"]->AddInput(externalResources["camera_buffer"]);

                renderPasses["GBufferPass"]->AddOutput(renderPassResourceMap["viewspace_normal"], {
                                                           ResourceUsage::Access::Write,
                                                           ResourceUsage::Type::Texture,
                                                           ResourceUsage::Usage::ColorAttachment
                                                       });

                renderPasses["GBufferPass"]->AddOutput(renderPassResourceMap["viewspace_position"], {
                                                           ResourceUsage::Access::Write,
                                                           ResourceUsage::Type::Texture,
                                                           ResourceUsage::Usage::ColorAttachment
                                                       });

                renderPasses["GBufferPass"]->AddOutput(renderPassResourceMap["depth_map"], {
                                                           ResourceUsage::Access::Write,
                                                           ResourceUsage::Type::Texture,
                                                           ResourceUsage::Usage::DepthStencil
                                                       });
            }

            // Lighting pass
            {
                renderPasses["LightingPass"]->AddInput(externalResources["camera_buffer"]);
                renderPasses["LightingPass"]->AddInput(externalResources["lights_buffer"]);
                renderPasses["LightingPass"]->AddInput(renderPassResourceMap["directional_shadow_map"]);
                renderPasses["LightingPass"]->AddInput(externalResources["irradiance_map_texture"]);
                renderPasses["LightingPass"]->AddInput(renderPassResourceMap["ssao_texture"]);
                renderPasses["LightingPass"]->AddInput(externalResources["prefilter_map_texture"]);
                renderPasses["LightingPass"]->AddInput(externalResources["brdflut_texture"]);


                renderPasses["LightingPass"]->AddOutput(renderPassResourceMap["hdr_image"], {
                                                            ResourceUsage::Access::ReadWrite,
                                                            ResourceUsage::Type::Texture,
                                                            ResourceUsage::Usage::ColorAttachment
                                                        });

                renderPasses["LightingPass"]->AddOutput(renderPassResourceMap["depth_map"], {
                                                            ResourceUsage::Access::Read,
                                                            ResourceUsage::Type::Texture,
                                                            ResourceUsage::Usage::DepthStencil
                                                        });
            }

            // SSAO pass
            {
                renderPasses["SSAOPass"]->AddInput(renderPassResourceMap["viewspace_normal"]);
                renderPasses["SSAOPass"]->AddInput(renderPassResourceMap["viewspace_position"]);
                renderPasses["SSAOPass"]->AddInput(renderPassResourceMap["depth_map"]);
                renderPasses["SSAOPass"]->AddInput(externalResources["camera_buffer"]);

                renderPasses["SSAOPass"]->AddOutput(renderPassResourceMap["ssao_texture"], {
                                                        ResourceUsage::Access::Write,
                                                        ResourceUsage::Type::Texture,
                                                        ResourceUsage::Usage::ColorAttachment
                                                    });
            }

            // Shadow map pass
            {
                renderPasses["ShadowMapPass"]->AddOutput(renderPassResourceMap["directional_shadow_map"], {
                                                             ResourceUsage::Access::Write,
                                                             ResourceUsage::Type::Texture,
                                                             ResourceUsage::Usage::ColorAttachment
                                                         });
            }

            // Tone Mapping pass
            {
                renderPasses["ToneMappingPass"]->AddInput(renderPassResourceMap["hdr_image"]);
                renderPasses["ToneMappingPass"]->AddOutput(renderPassResourceMap["main_frame_color"], {
                                                               ResourceUsage::Access::Write,
                                                               ResourceUsage::Type::Texture,
                                                               ResourceUsage::Usage::ColorAttachment
                                                           });
            }

            // UI pass
            {
                renderPasses["UiPass"]->AddOutput(renderPassResourceMap["main_frame_color"], {
                                                      ResourceUsage::Access::ReadWrite,
                                                      ResourceUsage::Type::Texture,
                                                      ResourceUsage::Usage::ColorAttachment
                                                  });
            }

            for (const auto& renderPass: renderPasses | std::views::values)
                renderPass->Init();
        }

        void FrameGraph::CreateFrameGraphOutputs()
        {
            // Viewspace Normal
            {
                TextureCreateInfo textureCreateInfo{};
                textureCreateInfo.resolution = resolution;
                textureCreateInfo.format = ImageFormat::RGBA32_SFLOAT;

                CreateFrameGraphTextureResource("viewspace_normal", textureCreateInfo);
            }

            // Viewspace Position
            {
                TextureCreateInfo textureCreateInfo{};
                textureCreateInfo.resolution = resolution;
                textureCreateInfo.format = ImageFormat::RGBA32_SFLOAT;

                CreateFrameGraphTextureResource("viewspace_position", textureCreateInfo);
            }

            // Depth Map
            {
                TextureCreateInfo textureCreateInfo{};
                textureCreateInfo.resolution = resolution;
                textureCreateInfo.format = ImageFormat::DEPTH24_STENCIL8;

                CreateFrameGraphTextureResource("depth_map", textureCreateInfo);
            }

            // Directional Shadow Map
            {
                constexpr uint16_t SHADOW_MAP_RESOLUTION = 4096;

                TextureCreateInfo textureCreateInfo{};
                textureCreateInfo.resolution = {SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION};
                textureCreateInfo.format = ImageFormat::DEPTH32;
                textureCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
                textureCreateInfo.arrayLayers = SHADOW_MAP_CASCADE_COUNT;
                textureCreateInfo.compareEnabled = true;
                textureCreateInfo.compareOp = VK_COMPARE_OP_LESS;

                CreateFrameGraphTextureResource("directional_shadow_map", textureCreateInfo);
            }

            // SSAO Texture
            {
                TextureCreateInfo textureCreateInfo;
                textureCreateInfo.resolution = VkExtent2D{
                    static_cast<uint32_t>(resolution.width * 0.5),
                    static_cast<uint32_t>(resolution.height * 0.5)
                };
                textureCreateInfo.format = ImageFormat::R8_UNORM;
                CreateFrameGraphTextureResource("ssao_texture", textureCreateInfo);
            }

            // HDR Image
            {
                TextureCreateInfo textureCreateInfo;
                textureCreateInfo.resolution = resolution;
                textureCreateInfo.format = ImageFormat::RGBA16_SFLOAT;

                CreateFrameGraphTextureResource("hdr_image", textureCreateInfo);
            }

            // Main Frame Color
            {
                TextureCreateInfo textureCreateInfo;
                textureCreateInfo.resolution = resolution;
                textureCreateInfo.format = ImageFormat::RGBA8_UNORM;

                CreateFrameGraphTextureResource("main_frame_color", textureCreateInfo);
            }
        }

        RenderPassHandle FrameGraph::CreateRenderPass(const std::vector<std::pair<FrameGraphResource*, ResourceUsage>>& outputs)
        {
            VulkanRenderPass::RenderPassConfig renderpassConfig{};

            for (const auto& [resource, usage]: outputs)
            {
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

            return device->CreateRenderPass(renderpassConfig);
        }

        PipelineHandle FrameGraph::CreatePipeline(PipelineCreateInfo pipelineCreate, RenderPassHandle renderPassHandle,
                                                  const std::vector<std::pair<FrameGraphResource*, ResourceUsage>>& outputs)
        {
            if (pipelineCreate.name == "" || pipelineCreate.vertexShaderPath == "" || pipelineCreate.fragmentShaderPath == "")
                ASSERT(false, "Invalid pipeline creation");

            LOG_TRACE(pipelineCreate.name);
            for (const auto& resource: outputs | std::views::keys)
            {
                ImageFormat format = resource->textureInfo.format;
                if (!IsDepthFormat(format))
                    pipelineCreate.colorAttachments.push_back(format);
                else
                    pipelineCreate.depthAttachment = format;
            }

            pipelineCreate.renderPass = device->renderPassPool.Get(renderPassHandle.handle)->Get();
            return VulkanPipelineBuilder().Build(device, pipelineCreate);
        }

        FramebufferHandle FrameGraph::CreateFramebuffer(RenderPassHandle renderPassHandle,
                                                        const std::vector<std::pair<FrameGraphResource*, ResourceUsage>>& outputs)
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

            return device->CreateFramebuffer(framebufferCreateInfo);
        }

        void FrameGraph::CreateFrameGraphTextureResource(const char* resourceName, const TextureCreateInfo& createInfo)
        {
            if (resourceHandles.contains(resourceName))
            {
                FrameGraphResource* resource = resourcePool.Get(resourceHandles[resourceName].index);

                device->DestroyTexture(resource->textureHandle);

                resourceHandles.erase(resourceName);
                resourcePool.Release(resource);
            }

            FrameGraphResource* graphResource = resourcePool.Obtain();
            graphResource->name = resourceName;
            graphResource->type = ResourceUsage::Type::Texture;
            graphResource->textureInfo = createInfo;
            graphResource->textureHandle = device->CreateTexture(createInfo);

            resourceHandles[graphResource->name] = {graphResource->index};
            renderPassResourceMap[graphResource->name] = graphResource;
        }

        void FrameGraph::CreateFrameGraphBufferResource(const char* resourceName, FrameGraphBufferCreateInfo& createInfo)
        {
            if (resourceHandles.contains(resourceName))
            {
                FrameGraphResource* resource = resourcePool.Get(resourceHandles[resourceName].index);

                device->DestroyBuffer(resource->allocatedBuffer);

                resourceHandles.erase(resourceName);
                resourcePool.Release(resource);
            }

            FrameGraphResource* graphResource = resourcePool.Obtain();
            graphResource->name = resourceName;
            graphResource->type = ResourceUsage::Type::Buffer;
            graphResource->allocatedBuffer = device->CreateBuffer(
                createInfo.size,
                createInfo.usageFlags,
                createInfo.memoryUsage
            );

            resourceHandles[graphResource->name] = {graphResource->index};
            renderPassResourceMap[graphResource->name] = graphResource;
        }
    }
}
