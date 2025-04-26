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
            Utils::DrawFloatControl("FOV", cameraFov, 0.0f, 180.0f, 0.1f, 45.0f, 150.0f);
            Utils::DrawFloatControl("Move speed", controller.movementSpeed, 0.0f, 10.0f, 0.001f, 1.0f, 150.0f);

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
            framebuffer = renderer->GetVulkanDevice()->GetFramebuffer();

            sampler = ImageSamplerBuilder(renderer->GetVulkanDevice())
                    .Build();

            for (auto attachment: framebuffer->GetAttachments())
            {
                if (!attachment.image) continue;

                auto descriptorSet = ImGui_ImplVulkan_AddTexture(sampler, attachment.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                framebufferAttachments.push_back(descriptorSet);
            }
        }

        ~FramebufferViewer() override = default;

        virtual const char* GetTitle() override
        {
            return "Framebuffer Viewer";
        }

        virtual void Draw() override
        {
            VkDescriptorSet descriptorSet = framebufferAttachments[1];
            const ImVec2 availableSpace = ImGui::GetContentRegionAvail();

            const float aspect = static_cast<float>(framebuffer->GetHeight()) / static_cast<float>(framebuffer->GetWidth());
            const ImVec2 imageSize = {
                std::min(availableSpace.x, availableSpace.y),
                std::min(availableSpace.x, availableSpace.y) * aspect,
            };

            ImGui::Text("Resolution: %d x %d", framebuffer->GetWidth(), framebuffer->GetHeight());
            ImGui::Image((ImTextureID) descriptorSet, imageSize, ImVec2(0, 0), ImVec2(1, 1));
        }

    private:
        Ref<VulkanFramebuffer> framebuffer;
        std::vector<VkDescriptorSet> framebufferAttachments;
        VkSampler sampler = VK_NULL_HANDLE;
    };
}
