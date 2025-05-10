#pragma once;

#include <backends/imgui_impl_vulkan.h>

#include "imgui.h"
#include "renderer/vulkan/imgui_vulkan.h"
#include "renderer/vulkan/vulkan_framebuffer.h"

namespace Raytracing
{
    class PerformanceWindow : ImGuiWindow {
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

    class CameraSettingsWindow : ImGuiWindow {
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
            float cameraFov = camera->GetFOV();;
            ImageUtils::DrawFloatControl("FOV", cameraFov, 0.0f, 180.0f, 0.1f, 45.0f, 150.0f);
            ImageUtils::DrawFloatControl("Move speed", controller.movementSpeed, 0.0f, 10.0f, 0.001f, 1.0f, 150.0f);

            camera->SetFOV(cameraFov);
        }

    private:
        Camera* camera;
        CameraController& controller;
    };

    class FramebufferViewer : ImGuiWindow {
    public:
        struct FramebufferInfo {
            VkDescriptorSet descriptorSet;
            uint32_t width;
            uint32_t height;
        };

    public:
        explicit FramebufferViewer(const Ref<VulkanRenderer>& _renderer): ImGuiWindow(_renderer)
        {
            sampler = ImageSamplerBuilder(renderer->GetVulkanDevice()).Build();
        }

        ~FramebufferViewer() override
        {
            if (sampler) vkDestroySampler(renderer->GetVulkanDevice()->GetDevice(), sampler, nullptr);
        }

        void Init()
        {
            if (gbufferAttachments.size() > 0)
            {
                for (int i = 0; i < gbufferAttachments.size(); i++)
                    ImGui_ImplVulkan_RemoveTexture(gbufferAttachments[i]);
                gbufferAttachments.clear();
            }

            gbuffer = renderer->GetGBuffer();
            for (const auto attachment: gbuffer->GetAttachments())
            {
                if (!attachment.allocatedImage.image) continue;

                auto descriptorSet = ImGui_ImplVulkan_AddTexture(sampler, attachment.imageView, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);
                gbufferAttachments.push_back(descriptorSet);
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

            const float aspect = static_cast<float>(gbuffer->GetHeight()) / static_cast<float>(gbuffer->GetWidth());
            const ImVec2 imageSize = {
                std::min(availableSpace.x, availableSpace.y),
                std::min(availableSpace.x, availableSpace.y) * aspect,
            };

            ImGui::Text("Resolution: %d x %d", gbuffer->GetWidth(), gbuffer->GetHeight());

            ImGui::Text("Base color:");
            ImGui::Image(reinterpret_cast<ImTextureID>(gbufferAttachments[0]), imageSize, ImVec2(0, 0), ImVec2(1, 1));

            ImGui::Text("Worldspace normal:");
            ImGui::Image(reinterpret_cast<ImTextureID>(gbufferAttachments[1]), imageSize, ImVec2(0, 0), ImVec2(1, 1));

            ImGui::Text("Metallic-Roughness:");
            ImGui::Image(reinterpret_cast<ImTextureID>(gbufferAttachments[2]), imageSize, ImVec2(0, 0), ImVec2(1, 1));

            ImGui::Text("Position:");
            ImGui::Image(reinterpret_cast<ImTextureID>(gbufferAttachments[3]), imageSize, ImVec2(0, 0), ImVec2(1, 1));
        }

        virtual void Resize() override
        {
            Init();
        }

    private:
        Ref<VulkanFramebuffer> gbuffer;
        std::vector<VkDescriptorSet> gbufferAttachments{};
        VkSampler sampler = VK_NULL_HANDLE;
    };

    class ShadowMapViewer : ImGuiWindow {
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
                    ImGui_ImplVulkan_RemoveTexture(shadowMapAttachments[i]);
                shadowMapAttachments.clear();
            }

            shadowMap = renderer->GetShadowMap();

            if (shadowMap->GetImage())
            {
                auto descriptorSet = ImGui_ImplVulkan_AddTexture(sampler, shadowMap->GetImageView(), VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);
                shadowMapAttachments.push_back(descriptorSet);
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
                std::min(availableSpace.x, availableSpace.y),
                std::min(availableSpace.x, availableSpace.y),
            };

            if (shadowMapAttachments.size() > 0)
            {
                ImGui::Image(reinterpret_cast<ImTextureID>(shadowMapAttachments[0]), imageSize, ImVec2(0, 0), ImVec2(1, 1));
            }
        }

        virtual void Resize() override
        {
            Init();
        }

    private:
        Ref<VulkanShadowMap> shadowMap;
        std::vector<VkDescriptorSet> shadowMapAttachments{};
        VkSampler sampler = VK_NULL_HANDLE;
    };

    class LightSettingsWindow : ImGuiWindow {
    public:
        explicit LightSettingsWindow(const Ref<VulkanRenderer>& _renderer): ImGuiWindow(_renderer), light(renderer->GetLight()) {}

        ~LightSettingsWindow() override = default;

        virtual const char* GetTitle() override
        {
            return "Light Settings";
        }

        virtual void Draw() override
        {
            ImageUtils::DrawVec3Control("Direction", light->direction, true, 1.0f, 150.0f);
            ImageUtils::DrawFloatControl("Intensity", light->intensity, 0.0f, 100.0f, 0.01f, 1.0f, 150.0f);
            ImageUtils::DrawFloatControl("Ambient Intensity", light->ambientIntensity, 0.0f, 100.0f, 0.01f, 0.01f, 150.0f);
            ImageUtils::DrawFloatControl("Near plane", light->nearPlane, 0.0f, 100.0f, 0.1f, 1.0f, 150.0f);
            ImageUtils::DrawFloatControl("Far plane", light->farPlane, 0.0f, 100.0f, 0.1f, 50.0f, 150.0f);
            ImageUtils::DrawFloatControl("Ortho size", light->orthoSize, 1.0f, 100.0f, 0.1f, 20.0f, 150.0f);
            ImageUtils::DrawFloatControl("Bias", light->bias, 0.0001f, 1.0f, 0.0001f, 0.005f, 150.0f);
        }

    private:
        DirectionalLight* light;
    };
}
