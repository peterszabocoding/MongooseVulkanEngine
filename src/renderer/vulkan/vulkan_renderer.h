#pragma once

#include "vulkan_cube_map_renderer.h"
#include "renderer/renderer.h"
#include "vulkan_device.h"
#include "vulkan_material.h"
#include "renderer/scene.h"
#include "renderer/transform.h"
#include "vulkan_swapchain.h"

namespace Raytracing
{
    struct DescriptorSetLayouts {
        Ref<VulkanDescriptorSetLayout> materialDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> lightingDescriptorSetLayout;
    };

    struct Pipelines {
        Ref<VulkanPipeline> geometryPipeline;
        Ref<VulkanPipeline> lightingPipeline;
    };

    class VulkanRenderer : public Renderer {
    public:
        VulkanRenderer() = default;
        ~VulkanRenderer() override = default;

        virtual void Init(int width, int height) override;

        virtual void ProcessPixel(unsigned int pixelCount, vec3 pixelColor) override {}
        virtual void OnRenderBegin(const Camera& camera) override {}
        virtual void OnRenderFinished(const Camera& camera) override {}

        virtual void IdleWait() override;
        virtual void Resize(int width, int height) override;
        virtual void DrawFrame(float deltaTime, Ref<Camera> camera) override;

        VulkanDevice* GetVulkanDevice() const { return vulkanDevice.get(); }

        [[nodiscard]] Ref<VulkanRenderPass> GetRenderPass() const { return gBufferPass; }
        [[nodiscard]] Ref<VulkanRenderPass> GetVulkanRenderPass() const { return gBufferPass; }
        [[nodiscard]] Ref<VulkanFramebuffer> GetFramebuffer() const { return gbufferFramebuffers[currentFrame]; }

        void OnDeviceReadyToResize();

    private:
        void CreateSwapchain();
        void CreateFramebuffers();
        void CreateRenderPasses();

        void ResizeSwapchain();

    public:
        static Pipelines pipelines;
        static DescriptorSetLayouts descriptorSetLayouts;

    private:
        uint32_t viewportWidth, viewportHeight;
        uint32_t currentFrame = 0;
        uint32_t currentImageIndex = 0;

        Scope<VulkanDevice> vulkanDevice;

        Ref<VulkanMesh> scene;
        Scene completeScene;
        Transform transform;

        Scope<VulkanMeshlet> screenRect;
        Scope<VulkanMeshlet> cube;

        Scope<VulkanSwapchain> vulkanSwapChain;

        std::vector<Ref<VulkanFramebuffer>> gbufferFramebuffers;
        std::vector<Ref<VulkanFramebuffer>> presentFramebuffers;
        std::array<Ref<VulkanFramebuffer>, 6> cubeMapFramebuffers;

        Ref<VulkanRenderPass> gBufferPass{};
        Ref<VulkanRenderPass> lightingPass{};
        Ref<VulkanRenderPass> cubeMapPass{};

        Ref<VulkanTexture> cubeMapTexture;
        VulkanCubeMapRenderer* cubeMapRenderer{};

        VkDescriptorSet presentDescriptorSet{};
        VkSampler presentSampler{};
    };
}
