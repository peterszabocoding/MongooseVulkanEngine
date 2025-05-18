#pragma once

#include "renderer/renderer.h"
#include "renderer/scene.h"
#include "renderer/transform.h"
#include "vulkan_device.h"
#include "vulkan_swapchain.h"
#include "vulkan_shadow_map.h"
#include "vulkan_cube_map_texture.h"
#include "vulkan_reflection_probe.h"
#include "pass/present_pass.h"
#include "pass/render_pass.h"
#include "pass/shadow_map_pass.h"
#include "renderer/Light.h"
#include "renderer/shader_cache.h"

namespace Raytracing
{
    struct DescriptorBuffers {
        Ref<VulkanBuffer> transformsBuffer{};
        Ref<VulkanBuffer> lightsBuffer{};
    };

    struct Framebuffers {
        std::vector<Ref<VulkanFramebuffer>> geometryFramebuffers;
        std::vector<Ref<VulkanShadowMap>> directionalShadowMaps;
        std::vector<Ref<VulkanFramebuffer>> shadowMapFramebuffers;
        std::vector<Ref<VulkanFramebuffer>> presentFramebuffers;
    };

    struct TransformsBuffer {
        glm::mat4 proj;
        glm::mat4 view;
        alignas(16) glm::vec3 cameraPosition;
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

    class VulkanRenderer final : public Renderer {
    public:
        VulkanRenderer() = default;
        ~VulkanRenderer() override;

        virtual void Init(int width, int height) override;
        void PrecomputeIBL();

        virtual void ProcessPixel(unsigned int pixelCount, vec3 pixelColor) override {}
        virtual void OnRenderBegin(const Camera& camera) override {}
        virtual void OnRenderFinished(const Camera& camera) override {}

        virtual void IdleWait() override;
        virtual void Resize(int width, int height) override;
        virtual void DrawFrame(float deltaTime, Ref<Camera> camera) override;

        VulkanDevice* GetVulkanDevice() const { return device.get(); }

        [[nodiscard]] Ref<VulkanRenderPass> GetRenderPass() const { return presentPass->GetRenderPass(); }
        [[nodiscard]] Ref<VulkanFramebuffer> GetGBuffer() const { return framebuffers.geometryFramebuffers[activeImage]; }
        [[nodiscard]] Ref<VulkanShadowMap> GetShadowMap() const { return framebuffers.directionalShadowMaps[activeImage]; }

        DirectionalLight* GetLight() { return &directionalLight; }

    private:
        void CreateSwapchain();
        void CreateFramebuffers();

        void ResizeSwapchain();
        void PreparePresentPass();

        void CreateTransformsBuffer();
        void UpdateTransformsBuffer(const Ref<Camera>& camera) const;

        void CreateLightsBuffer();
        void UpdateLightsBuffer(float deltaTime);

    public:
        DescriptorBuffers descriptorBuffers;
        Framebuffers framebuffers;

    private:
        uint32_t viewportWidth, viewportHeight;
        uint32_t activeImage = 0;

        float resolutionScale = 1.0f;

        Scope<VulkanDevice> device;
        Scope<ShaderCache> shaderCache;
        Scope<VulkanSwapchain> vulkanSwapChain;

        Scene scene;

        Scope<VulkanMeshlet> screenRect;
        Ref<VulkanMesh> cubeMesh;

        Ref<VulkanCubeMapTexture> irradianceMap;

        float lightSpinningAngle = 0.0f;
        DirectionalLight directionalLight;

        Scope<RenderPass> renderPass;
        Scope<ShadowMapPass> shadowMapPass;
        Scope<PresentPass> presentPass;
    };
}
