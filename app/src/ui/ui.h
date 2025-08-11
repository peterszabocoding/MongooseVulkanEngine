#pragma once

#include <backends/imgui_impl_vulkan.h>
#include <input/camera_controller.h>
#include <renderer/vulkan/vulkan_image.h>
#include <renderer/vulkan/vulkan_texture.h>

#include "imgui.h"
#include "renderer/vulkan/imgui_vulkan.h"

namespace VulkanDemo
{
    class PerformanceWindow final : MongooseVK::ImGuiWindow {
    public:
        PerformanceWindow(MongooseVK::VulkanRenderer& _renderer): ImGuiWindow(_renderer), device(MongooseVK::VulkanDevice::Get()) {}
        ~PerformanceWindow() override = default;

        virtual const char* GetTitle() override
        {
            return "Performance";
        }

        virtual void Draw() override
        {
            const auto io = ImGui::GetIO();
            const auto deviceProperties = device->GetDeviceProperties();

            ImGui::Text(deviceProperties.deviceName);
            ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::Text("%d draw calls", device->GetDrawCallCount(), io.Framerate);
        }

    private:
        MongooseVK::VulkanDevice* device;
    };

    class CameraSettingsWindow final : MongooseVK::ImGuiWindow {
    public:
        explicit CameraSettingsWindow(MongooseVK::VulkanRenderer& _renderer, MongooseVK::Camera* camera,
                                      MongooseVK::CameraController& controller): ImGuiWindow(_renderer),
                                                                                 camera(camera), controller(controller) {}

        ~CameraSettingsWindow() override = default;

        virtual const char* GetTitle() override
        {
            return "Camera Settings";
        }

        virtual void Draw() override
        {
            float cameraFov = camera->GetFOV();
            float nearPlane = camera->GetNearPlane();
            float farPlane = camera->GetFarPlane();
            MongooseVK::ImGuiUtils::DrawFloatControl("FOV", cameraFov, 0.0f, 180.0f, 0.1f, 150.0f);
            MongooseVK::ImGuiUtils::DrawFloatControl("Near Plane", nearPlane, -100.0f, 100.0f, 0.1f, 150.0f);
            MongooseVK::ImGuiUtils::DrawFloatControl("Far Plane", farPlane, -100.0f, 100.0f, 0.1f, 150.0f);
            MongooseVK::ImGuiUtils::DrawFloatControl("Move speed", controller.movementSpeed, 0.0f, 10.0f, 0.001f, 150.0f);

            camera->SetFOV(cameraFov);
            camera->SetNearPlane(nearPlane);
            camera->SetFarPlane(farPlane);
        }

    private:
        MongooseVK::Camera* camera;
        MongooseVK::CameraController& controller;
    };

    class GBufferViewer final : MongooseVK::ImGuiWindow {
    public:
        struct FramebufferInfo {
            VkDescriptorSet descriptorSet;
            uint32_t width;
            uint32_t height;
        };

    public:
        explicit GBufferViewer(MongooseVK::VulkanRenderer& _renderer): ImGuiWindow(_renderer)
        {
            sampler = MongooseVK::ImageSamplerBuilder(renderer.GetVulkanDevice()).Build();
            depthSampler = MongooseVK::ImageSamplerBuilder(renderer.GetVulkanDevice())
                           .SetFormat(MongooseVK::ImageFormat::DEPTH24_STENCIL8)
                           .Build();
        }

        ~GBufferViewer() override
        {
            if (sampler) vkDestroySampler(renderer.GetVulkanDevice()->GetDevice(), sampler, nullptr);
            if (depthSampler) vkDestroySampler(renderer.GetVulkanDevice()->GetDevice(), depthSampler, nullptr);
        }

        void Init()
        {
            if (debugTextures.size() > 0)
            {
                for (int i = 0; i < debugTextures.size(); i++)
                {
                    ImGui_ImplVulkan_RemoveTexture(debugTextures[i]);
                }
                debugTextures.clear();
            }


            // Viewspace Normal
            {
                auto viewspaceNormal = MongooseVK::VulkanDevice::Get()->GetTexture(
                    renderer.renderPassResourceMap["viewspace_normal"].resourceInfo.texture.textureHandle);

                debugTextures.push_back(ImGui_ImplVulkan_AddTexture(sampler,
                                                                    viewspaceNormal->imageView,
                                                                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
            }

            // Viewspace Position
            {
                auto viewspacePosition = MongooseVK::VulkanDevice::Get()->GetTexture(
                    renderer.renderPassResourceMap["viewspace_position"].resourceInfo.texture.textureHandle);

                debugTextures.push_back(ImGui_ImplVulkan_AddTexture(sampler,
                                                                    viewspacePosition->imageView,
                                                                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
            }


            // Depth map
            {
                auto depthMap = MongooseVK::VulkanDevice::Get()->GetTexture(
                    renderer.renderPassResourceMap["depth_map"].resourceInfo.texture.textureHandle);

                debugTextures.push_back(ImGui_ImplVulkan_AddTexture(depthSampler,
                                                                    depthMap->imageView,
                                                                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL));
            }

            // SSAO
            {
                auto ssao = MongooseVK::VulkanDevice::Get()->GetTexture(
                    renderer.renderPassResourceMap["ssao_texture"].resourceInfo.texture.textureHandle);

                debugTextures.push_back(ImGui_ImplVulkan_AddTexture(sampler,
                                                                    ssao->imageView,
                                                                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
            }
        }

        virtual const char* GetTitle() override
        {
            return "GBuffer Viewer";
        }

        virtual void Draw() override
        {
            Init();

            const ImVec2 availableSpace = ImGui::GetContentRegionAvail();

            const float aspect = static_cast<float>(renderer.renderResolution.height) / static_cast<float>(renderer.renderResolution.width);
            const ImVec2 imageSize = {
                std::min(availableSpace.x, availableSpace.y),
                std::min(availableSpace.x, availableSpace.y) * aspect,
            };

            ImGui::Text("Resolution: %d x %d", renderer.renderResolution.width, renderer.renderResolution.height);

            ImGui::Text("Worldspace normal:");
            ImGui::Image(reinterpret_cast<ImTextureID>(debugTextures[0]), imageSize, ImVec2(0, 0), ImVec2(1, 1));

            ImGui::Text("Position:");
            ImGui::Image(reinterpret_cast<ImTextureID>(debugTextures[1]), imageSize, ImVec2(0, 0), ImVec2(1, 1));

            ImGui::Text("Depth Map:");
            ImGui::Image(reinterpret_cast<ImTextureID>(debugTextures[2]), imageSize, ImVec2(0, 0), ImVec2(1, 1));

            ImGui::Text("SSAO:");
            ImGui::Image(reinterpret_cast<ImTextureID>(debugTextures[3]), imageSize, ImVec2(0, 0), ImVec2(1, 1));
        }

        virtual void Resize() override
        {
            Init();
        }

    private:
        std::vector<VkDescriptorSet> debugTextures{};
        VkSampler sampler = VK_NULL_HANDLE;
        VkSampler depthSampler = VK_NULL_HANDLE;
    };

    class ShadowMapViewer final : MongooseVK::ImGuiWindow {
    public:
        struct FramebufferInfo {
            VkDescriptorSet descriptorSet;
            uint32_t width;
            uint32_t height;
        };

    public:
        explicit ShadowMapViewer(MongooseVK::VulkanRenderer& _renderer): ImGuiWindow(_renderer)
        {
            sampler = MongooseVK::ImageSamplerBuilder(renderer.GetVulkanDevice()).Build();
        }

        ~ShadowMapViewer() override
        {
            if (sampler) vkDestroySampler(renderer.GetVulkanDevice()->GetDevice(), sampler, nullptr);
        }

        void Init()
        {
            if (shadowMapAttachments.size() > 0)
            {
                for (int i = 0; i < shadowMapAttachments.size(); i++)
                {
                    ImGui_ImplVulkan_RemoveTexture(shadowMapAttachments[i]);
                }
                shadowMapAttachments.clear();
            }

            MongooseVK::VulkanTexture* shadowMap = MongooseVK::VulkanDevice::Get()->GetTexture(
                renderer.renderPassResourceMap["directional_shadow_map"].resourceInfo.texture.textureHandle);
            if (shadowMap->GetImage())
            {
                for (size_t i = 0; i < MongooseVK::SHADOW_MAP_CASCADE_COUNT; i++)
                {
                    shadowMapAttachments.push_back(ImGui_ImplVulkan_AddTexture(sampler, shadowMap->GetImageView(i),
                                                                               VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL));
                }
            }
        }

        virtual const char* GetTitle() override
        {
            return "Shadow Map Viewer";
        }

        virtual void Draw() override
        {
            Init();

            const ImVec2 availableSpace = ImGui::GetContentRegionAvail();
            const ImVec2 imageSize = {
                availableSpace.x,
                availableSpace.x,
            };

            for (auto attachment: shadowMapAttachments)
            {
                ImGui::Image(reinterpret_cast<ImTextureID>(attachment), imageSize, ImVec2(0, 0), ImVec2(1, 1));
            }
        }

        virtual void Resize() override
        {
            Init();
        }

    private:
        std::vector<VkDescriptorSet> shadowMapAttachments{};
        VkSampler sampler = VK_NULL_HANDLE;
    };

    class LightSettingsWindow final : MongooseVK::ImGuiWindow {
    public:
        explicit LightSettingsWindow(MongooseVK::VulkanRenderer& _renderer): ImGuiWindow(_renderer), light(renderer.GetLight()) {}

        ~LightSettingsWindow() override = default;

        virtual const char* GetTitle() override
        {
            return "Light Settings";
        }

        virtual void Draw() override
        {
            MongooseVK::ImGuiUtils::DrawVec3Control("Direction", light->direction, true, 1.0f, 150.0f);
            MongooseVK::ImGuiUtils::DrawVec3Control("Center", light->center, false, 0.0f, 150.0f);
            MongooseVK::ImGuiUtils::DrawFloatControl("Intensity", light->intensity, 0.0f, 100.0f, 0.01f, 150.0f);
            MongooseVK::ImGuiUtils::DrawFloatControl("Ambient Intensity", light->ambientIntensity, 0.0f, 100.0f, 0.01f, 150.0f);
            MongooseVK::ImGuiUtils::DrawFloatControl("Cascade split lambda", light->cascadeSplitLambda, 0.01f, 10.0f, 0.01f, 150.0f);
        }

    private:
        MongooseVK::DirectionalLight* light;
    };

    class PostProcessingWindow final : MongooseVK::ImGuiWindow {
    public:
        explicit PostProcessingWindow(MongooseVK::VulkanRenderer& _renderer): ImGuiWindow(_renderer) {}
        ~PostProcessingWindow() override = default;

        virtual const char* GetTitle() override
        {
            return "Post Processing";
        }

        virtual void Draw() override
        {
            const auto ssaoPass = static_cast<MongooseVK::SSAOPass*>(renderer.frameGraphRenderPasses["SSAOPass"]);
            const auto toneMappingPass = static_cast<MongooseVK::ToneMappingPass*>(renderer.frameGraphRenderPasses["ToneMappingPass"]);

            MongooseVK::ImGuiUtils::DrawFloatControl("SSAO Strength", ssaoPass->ssaoParams.strength, 0.01f, 10.0f,
                                                     0.01f, 150.0f);
            MongooseVK::ImGuiUtils::DrawFloatControl("SSAO Radius", ssaoPass->ssaoParams.radius, 0.01f, 1.0f, 0.01f,
                                                     150.0f);
            MongooseVK::ImGuiUtils::DrawFloatControl("SSAO Bias", ssaoPass->ssaoParams.bias, 0.001f, 1.0f, 0.001f,
                                                     150.0f);
            MongooseVK::ImGuiUtils::DrawIntControl("SSAO Kernel Size", ssaoPass->ssaoParams.kernelSize, 1, 64,
                                                   150.0f);

            MongooseVK::ImGuiUtils::DrawFloatControl("Gamma", toneMappingPass->toneMappingParams.gamma, 0.001f, 10.0f, 0.01f,
                                                     150.0f);
            MongooseVK::ImGuiUtils::DrawFloatControl("Exposure", toneMappingPass->toneMappingParams.exposure, 0.01f, 10.0f, 0.01f,
                                                     150.0f);
        }
    };

    class GridSettingsWindow final : MongooseVK::ImGuiWindow {
    public:
        explicit GridSettingsWindow(MongooseVK::VulkanRenderer& _renderer): ImGuiWindow(_renderer) {}
        ~GridSettingsWindow() override = default;

        virtual const char* GetTitle() override
        {
            return "Grid Settings";
        }

        virtual void Draw() override
        {
            const auto gridPass = static_cast<MongooseVK::InfiniteGridPass*>(renderer.frameGraphRenderPasses["InfiniteGridPass"]);

            MongooseVK::ImGuiUtils::DrawFloatControl("Grid size", gridPass->gridParams.gridSize, 0.1f, 1000.0f, 0.1f, 150.0f);
            MongooseVK::ImGuiUtils::DrawFloatControl("Cell size", gridPass->gridParams.gridCellSize, 0.01f, 10.0f, 0.01f, 150.0f);
            MongooseVK::ImGuiUtils::DrawRGBColorPicker("Primary color", gridPass->gridParams.gridColorThick, 150.0f);
            MongooseVK::ImGuiUtils::DrawRGBColorPicker("Secondary color", gridPass->gridParams.gridColorThin, 150.0f);
        }
    };
}
