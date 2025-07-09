#pragma once;

#include <backends/imgui_impl_vulkan.h>

#include "imgui.h"
#include "renderer/vulkan/imgui_vulkan.h"
#include "renderer/vulkan/vulkan_framebuffer.h"

namespace MongooseVK
{
    class PerformanceWindow final : ImGuiWindow {
    public:
        PerformanceWindow(const Ref<VulkanRenderer>& _renderer): ImGuiWindow(_renderer), device(renderer->GetVulkanDevice()) {}
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
        }

    private:
        VulkanDevice* device;
    };

    class CameraSettingsWindow final : ImGuiWindow {
    public:
        explicit CameraSettingsWindow(const Ref<VulkanRenderer>& _renderer, Camera* camera,
                                      CameraController& controller): ImGuiWindow(_renderer),
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
            ImGuiUtils::DrawFloatControl("FOV", cameraFov, 0.0f, 180.0f, 0.1f, 150.0f);
            ImGuiUtils::DrawFloatControl("Near Plane", nearPlane, -100.0f, 100.0f, 0.1f, 150.0f);
            ImGuiUtils::DrawFloatControl("Far Plane", farPlane, -100.0f, 100.0f, 0.1f, 150.0f);
            ImGuiUtils::DrawFloatControl("Move speed", controller.movementSpeed, 0.0f, 10.0f, 0.001f, 150.0f);

            camera->SetFOV(cameraFov);
            camera->SetNearPlane(nearPlane);
            camera->SetFarPlane(farPlane);
        }

    private:
        Camera* camera;
        CameraController& controller;
    };

    class GBufferViewer final : ImGuiWindow {
    public:
        struct FramebufferInfo {
            VkDescriptorSet descriptorSet;
            uint32_t width;
            uint32_t height;
        };

    public:
        explicit GBufferViewer(const Ref<VulkanRenderer>& _renderer): ImGuiWindow(_renderer)
        {
            sampler = ImageSamplerBuilder(renderer->GetVulkanDevice()).Build();
        }

        ~GBufferViewer() override
        {
            if (sampler) vkDestroySampler(renderer->GetVulkanDevice()->GetDevice(), sampler, nullptr);
        }

        void Init()
        {
            if (debugTextures.size() > 0)
            {
                for (int i = 0; i < debugTextures.size(); i++)
                    ImGui_ImplVulkan_RemoveTexture(debugTextures[i]);
                debugTextures.clear();
            }

            gBuffer = renderer->gbufferPass->gBuffer;
            ssaoBuffer = renderer->ssaoPass->framebuffer;

            debugTextures.push_back(ImGui_ImplVulkan_AddTexture(sampler, gBuffer->buffers.viewSpaceNormal.imageView,
                                                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
            debugTextures.push_back(ImGui_ImplVulkan_AddTexture(sampler, gBuffer->buffers.viewSpacePosition.imageView,
                                                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
            debugTextures.push_back(
                ImGui_ImplVulkan_AddTexture(sampler, gBuffer->buffers.depth.imageView, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));

            auto ssaoDescriptorSet = ImGui_ImplVulkan_AddTexture(sampler, ssaoBuffer->GetAttachments()[0].imageView,
                                                                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            debugTextures.push_back(ssaoDescriptorSet);
        }

        virtual const char* GetTitle() override
        {
            return "GBuffer Viewer";
        }

        virtual void Draw() override
        {
            Init();

            const ImVec2 availableSpace = ImGui::GetContentRegionAvail();

            const float aspect = static_cast<float>(gBuffer->height) / static_cast<float>(gBuffer->width);
            const ImVec2 imageSize = {
                std::min(availableSpace.x, availableSpace.y),
                std::min(availableSpace.x, availableSpace.y) * aspect,
            };

            ImGui::Text("Resolution: %d x %d", gBuffer->width, gBuffer->height);

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
        Ref<VulkanGBuffer> gBuffer;
        Ref<VulkanFramebuffer> ssaoBuffer;
        std::vector<VkDescriptorSet> debugTextures{};
        VkSampler sampler = VK_NULL_HANDLE;
    };

    class ShadowMapViewer final : ImGuiWindow {
    public:
        struct FramebufferInfo {
            VkDescriptorSet descriptorSet;
            uint32_t width;
            uint32_t height;
        };

    public:
        explicit ShadowMapViewer(const Ref<VulkanRenderer>& _renderer): ImGuiWindow(_renderer)
        {
            sampler = ImageSamplerBuilder(renderer->GetVulkanDevice()).Build();
        }

        ~ShadowMapViewer() override
        {
            if (sampler) vkDestroySampler(renderer->GetVulkanDevice()->GetDevice(), sampler, nullptr);
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

            if (renderer->directionalShadowMap->GetImage())
            {
                for (size_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
                {
                    shadowMapAttachments.push_back(ImGui_ImplVulkan_AddTexture(sampler, renderer->directionalShadowMap->GetImageView(i),
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

            for (auto attachment : shadowMapAttachments)
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

    class LightSettingsWindow final : ImGuiWindow {
    public:
        explicit LightSettingsWindow(const Ref<VulkanRenderer>& _renderer): ImGuiWindow(_renderer), light(renderer->GetLight()) {}

        ~LightSettingsWindow() override = default;

        virtual const char* GetTitle() override
        {
            return "Light Settings";
        }

        virtual void Draw() override
        {
            ImGuiUtils::DrawVec3Control("Direction", light->direction, true, 1.0f, 150.0f);
            ImGuiUtils::DrawVec3Control("Center", light->center, false, 0.0f, 150.0f);
            ImGuiUtils::DrawFloatControl("Intensity", light->intensity, 0.0f, 100.0f, 0.01f, 150.0f);
            ImGuiUtils::DrawFloatControl("Ambient Intensity", light->ambientIntensity, 0.0f, 100.0f, 0.01f, 150.0f);
            ImGuiUtils::DrawFloatControl("Cascade split lambda", light->cascadeSplitLambda, 0.01f, 10.0f, 0.01f, 150.0f);
        }

    private:
        DirectionalLight* light;
    };

    class PostProcessingWindow final : ImGuiWindow {
    public:
        explicit PostProcessingWindow(const Ref<VulkanRenderer>& _renderer): ImGuiWindow(_renderer) {}
        ~PostProcessingWindow() override = default;

        virtual const char* GetTitle() override
        {
            return "Post Processing";
        }

        virtual void Draw() override
        {
            ImGuiUtils::DrawFloatControl("SSAO Strength", renderer->ssaoPass->ssaoParams.strength, 0.01f, 10.0f, 0.01f, 150.0f);
            ImGuiUtils::DrawFloatControl("SSAO Radius", renderer->ssaoPass->ssaoParams.radius, 0.01f, 1.0f, 0.01f, 150.0f);
            ImGuiUtils::DrawFloatControl("SSAO Bias", renderer->ssaoPass->ssaoParams.bias, 0.001f, 1.0f, 0.001f, 150.0f);
            ImGuiUtils::DrawIntControl("SSAO Kernel Size", renderer->ssaoPass->ssaoParams.kernelSize, 1, 64, 150.0f);
        }
    };

    class GridSettingsWindow final : ImGuiWindow {
    public:
        explicit GridSettingsWindow(const Ref<VulkanRenderer>& _renderer): ImGuiWindow(_renderer) {}
        ~GridSettingsWindow() override = default;

        virtual const char* GetTitle() override
        {
            return "Grid Settings";
        }

        virtual void Draw() override
        {
            ImGuiUtils::DrawFloatControl("Grid size", renderer->gridPass->gridParams.gridSize, 0.1f, 1000.0f, 0.1f, 150.0f);
            ImGuiUtils::DrawFloatControl("Cell size", renderer->gridPass->gridParams.gridCellSize, 0.01f, 10.0f, 0.01f, 150.0f);
            ImGuiUtils::DrawRGBColorPicker("Primary color", renderer->gridPass->gridParams.gridColorThick, 150.0f);
            ImGuiUtils::DrawRGBColorPicker("Secondary color", renderer->gridPass->gridParams.gridColorThin, 150.0f);
        }
    };
}
