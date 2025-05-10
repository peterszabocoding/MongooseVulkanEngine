#pragma once

#include "renderer/renderer.h"
#include "renderer/scene.h"
#include "renderer/transform.h"
#include "vulkan_device.h"
#include "vulkan_material.h"
#include "vulkan_swapchain.h"
#include "vulkan_shadow_map.h"
#include "vulkan_cube_map_texture.h"
#include "renderer/Light.h"

namespace Raytracing
{
    struct DescriptorSetLayouts {
        Ref<VulkanDescriptorSetLayout> skyboxDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> materialDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> transformDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> lightsDescriptorSetLayout;
    };

    struct DescriptorSets {
        VkDescriptorSet skyboxDescriptorSet;
        VkDescriptorSet transformDescriptorSet;
        std::vector<VkDescriptorSet> lightsDescriptorSets;
    };

    struct DescriptorBuffers {
        Ref<VulkanBuffer> transformsBuffer{};
        Ref<VulkanBuffer> lightsBuffer{};
    };

    struct Pipelines {
        Ref<VulkanPipeline> skyBox;
        Ref<VulkanPipeline> geometry;
        Ref<VulkanPipeline> directionalShadowMap;
    };

    struct Renderpass {
        Ref<VulkanRenderPass> skyboxPass{};
        Ref<VulkanRenderPass> geometryPass{};
        Ref<VulkanRenderPass> shadowMapPass{};
    };

    struct TransformsBuffer {
        glm::vec4 cameraPosition;
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct LightsBuffer {
        glm::mat4 lightView;
        glm::mat4 lightProjection;
        alignas(16) glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
        alignas(4) float ambientIntensity = 0.1f;
        glm::vec4 color = glm::vec4(1.0f);
        alignas(4) float intensity = 1.0f;
        alignas(4) float bias = 0.005f;
    };

    class VulkanRenderer : public Renderer {
    public:
        VulkanRenderer() = default;
        ~VulkanRenderer() override;

        virtual void Init(int width, int height) override;

        virtual void ProcessPixel(unsigned int pixelCount, vec3 pixelColor) override {}
        virtual void OnRenderBegin(const Camera& camera) override {}
        virtual void OnRenderFinished(const Camera& camera) override {}

        virtual void IdleWait() override;
        virtual void Resize(int width, int height) override;
        virtual void DrawFrame(float deltaTime, Ref<Camera> camera) override;

        VulkanDevice* GetVulkanDevice() const { return vulkanDevice.get(); }

        [[nodiscard]] Ref<VulkanRenderPass> GetRenderPass() const { return renderpasses.geometryPass; }
        [[nodiscard]] Ref<VulkanFramebuffer> GetGBuffer() const { return geometryFramebuffers[activeImage]; }
        [[nodiscard]] Ref<VulkanShadowMap> GetShadowMap() const { return directionalShadowMaps[activeImage]; }

        DirectionalLight* GetLight() { return &directionalLight; }

    private:
        void DrawDirectionalShadowMapPass(VkCommandBuffer commandBuffer);
        void DrawSkybox(VkCommandBuffer commandBuffer) const;
        void DrawGeometryPass(const Ref<Camera>& camera, VkCommandBuffer commandBuffer) const;

    private:
        void CreateSwapchain();
        void CreateFramebuffers();
        void CreateRenderPasses() const;

        void ResizeSwapchain();
        void CreateTransformsBuffer();
        void CreateLightsBuffer();
        void PrepareSkyboxPass();
        void PrepareLightsDescriptorSet();

        void UpdateTransformsBuffer(const Ref<Camera>& camera) const;
        void UpdateLightsBuffer(float deltaTime);

    public:
        static Pipelines pipelines;
        static DescriptorSetLayouts descriptorSetLayouts;
        static DescriptorBuffers descriptorBuffers;
        static DescriptorSets descriptorSets;
        static Renderpass renderpasses;

    private:
        uint32_t viewportWidth, viewportHeight;
        uint32_t activeImage = 0;

        Scope<VulkanDevice> vulkanDevice;

        Ref<VulkanMesh> scene;
        Scene completeScene;

        Scope<VulkanMeshlet> screenRect;
        Ref<VulkanMesh> cubeMesh;

        Scope<VulkanSwapchain> vulkanSwapChain;

        std::vector<Ref<VulkanFramebuffer>> geometryFramebuffers;
        std::vector<Ref<VulkanShadowMap>> directionalShadowMaps;
        std::vector<Ref<VulkanFramebuffer>> shadowMapFramebuffers;

        Ref<VulkanCubeMapTexture> cubemapTexture;

        float lightSpinningAngle = 0.0f;
        DirectionalLight directionalLight;
    };
}
