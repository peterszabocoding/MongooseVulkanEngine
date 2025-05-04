#pragma once

#include "vulkan_cube_map_texture.h"
#include "renderer/renderer.h"
#include "vulkan_device.h"
#include "vulkan_material.h"
#include "renderer/scene.h"
#include "renderer/transform.h"
#include "vulkan_swapchain.h"

namespace Raytracing
{
    struct DescriptorSetLayouts {
        Ref<VulkanDescriptorSetLayout> skyboxDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> materialDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> lightingDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> transformDescriptorSetLayout;
    };

    struct Pipelines {
        Ref<VulkanPipeline> skyBox;
        Ref<VulkanPipeline> gBuffer;
        Ref<VulkanPipeline> lighting;
    };

    struct Renderpass {
        Ref<VulkanRenderPass> cubeMapPass{};
        Ref<VulkanRenderPass> skyboxPass{};
        Ref<VulkanRenderPass> gBufferPass{};
        Ref<VulkanRenderPass> lightingPass{};
    };

    struct TransformsBuffer {
        glm::vec4 cameraPosition;
        glm::mat4 view;
        glm::mat4 proj;
    };

    class VulkanRenderer : public Renderer {
    public:
        VulkanRenderer() = default;
        ~VulkanRenderer() override;

        virtual void Init(int width, int height) override;
        void DrawGeometryPass(float deltaTime, const Ref<Camera>& camera, VkCommandBuffer commandBuffer) const;
        void DrawLightingPass(VkCommandBuffer commandBuffer);

        virtual void ProcessPixel(unsigned int pixelCount, vec3 pixelColor) override {}
        virtual void OnRenderBegin(const Camera& camera) override {}
        virtual void OnRenderFinished(const Camera& camera) override {}

        virtual void IdleWait() override;
        virtual void Resize(int width, int height) override;
        virtual void DrawFrame(float deltaTime, Ref<Camera> camera) override;

        VulkanDevice* GetVulkanDevice() const { return vulkanDevice.get(); }

        [[nodiscard]] Ref<VulkanRenderPass> GetRenderPass() const { return renderpass.gBufferPass; }
        [[nodiscard]] Ref<VulkanFramebuffer> GetGBuffer() const { return gbufferFramebuffers[activeImage]; }

    private:
        void CreateSwapchain();
        void CreateFramebuffers();
        void CreateRenderPasses();

        void ResizeSwapchain();
        void CreateTransformsBuffer();
        void UpdateTransformsBuffer(const Ref<Camera>& camera);
        void PrepareLightingPass();
        void PrepareSkyboxPass();

    public:
        static Pipelines pipelines;
        static DescriptorSetLayouts descriptorSetLayouts;
        static Renderpass renderpass;

    private:
        uint32_t viewportWidth, viewportHeight;
        uint32_t activeImage = 0;

        Scope<VulkanDevice> vulkanDevice;

        Ref<VulkanMesh> scene;
        Scene completeScene;
        Transform transform;

        Scope<VulkanMeshlet> screenRect;
        //Scope<VulkanMeshlet> cube;
        Ref<VulkanMesh> cubeMesh;

        Scope<VulkanSwapchain> vulkanSwapChain;

        std::vector<Ref<VulkanFramebuffer>> gbufferFramebuffers;
        std::vector<Ref<VulkanFramebuffer>> presentFramebuffers;
        std::array<Ref<VulkanFramebuffer>, 6> cubeMapFramebuffers;

        Ref<VulkanTexture> hdrTexture;
        Ref<VulkanCubeMapTexture> cubemapTexture;

        uint32_t cubemapResolution = 0;

        std::vector<VkDescriptorSet> presentDescriptorSets{};
        VkSampler presentSampler{};

        Ref<VulkanBuffer> transformsBuffer{};

        VkDescriptorSet skyboxDescriptorSet;
        VkDescriptorSet transformDescriptorSet;
    };
}
