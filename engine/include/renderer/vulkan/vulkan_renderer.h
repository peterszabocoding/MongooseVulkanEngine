#pragma once

#include "renderer/scene.h"
#include "vulkan_device.h"
#include "vulkan_swapchain.h"
#include "vulkan_shadow_map.h"
#include "vulkan_cube_map_texture.h"
#include "pass/gbufferPass.h"
#include "pass/infinite_grid_pass.h"
#include "pass/lighting_pass.h"
#include "pass/present_pass.h"
#include "pass/shadow_map_pass.h"
#include "pass/skybox_pass.h"
#include "pass/post_processing/ssao_pass.h"
#include "renderer/Light.h"
#include "renderer/shader_cache.h"

namespace MongooseVK
{
    struct DescriptorBuffers {
        AllocatedBuffer cameraBuffer{};
        AllocatedBuffer lightsBuffer{};
    };

    struct Framebuffers {
        Ref<VulkanFramebuffer> ssaoFramebuffer;
        Ref<VulkanFramebuffer> gbufferFramebuffer;
        Ref<VulkanFramebuffer> geometryFramebuffer;
        std::vector<Ref<VulkanFramebuffer>> shadowMapFramebuffers;
        std::vector<Ref<VulkanFramebuffer>> presentFramebuffers;
    };

    struct TransformsBuffer {
        glm::mat4 proj;
        glm::mat4 view;
        alignas(16) glm::vec3 cameraPosition;
    };

    struct LightsBuffer {
        glm::mat4 lightProjection[SHADOW_MAP_CASCADE_COUNT];
        glm::vec4 color = glm::vec4(1.0f);
        glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
        float ambientIntensity = 0.1f;
        alignas(4) float cascadeSplits[4];
        alignas(4) float intensity = 1.0f;
        alignas(4) float bias = 0.005f;
    };

    struct SceneDefinition {
        std::string gltfPath;
        std::string hdrPath;
    };

    struct RenderPassResources {
        PassResource skyboxTexture;
        PassResource viewspacePosition;
        PassResource viewspaceNormal;
        PassResource depthMap;
        PassResource cameraBuffer;
        PassResource lightsBuffer;
        PassResource directionalShadowMap;
        PassResource irradianceMap;
        PassResource ssaoTexture;
    };

    struct RenderPasses {
        Scope<GBufferPass> gbufferPass;
        Scope<SkyboxPass> skyboxPass;
        Scope<LightingPass> lightingPass;
        Scope<ShadowMapPass> shadowMapPass;
        Scope<PresentPass> presentPass;
        Scope<SSAOPass> ssaoPass;
        Scope<InfiniteGridPass> gridPass;
    };

    class VulkanRenderer {
    public:
        VulkanRenderer() = default;
        ~VulkanRenderer();

        void Init(uint32_t width, uint32_t height);

        void LoadScene(const std::string& gltfPath, const std::string& hdrPath);

        void PrecomputeIBL();
        void IdleWait();
        void Resize(int width, int height);
        void Draw(float deltaTime, Camera& camera);

        VulkanDevice* GetVulkanDevice() const { return device; }

        [[nodiscard]] Ref<VulkanShadowMap> GetShadowMap() const { return directionalShadowMap; }

        DirectionalLight* GetLight() { return &scene.directionalLight; }

    private:
        void CreateSwapchain();
        void CreateShadowMap();
        void CreateFramebuffers();

        void ResizeSwapchain();
        void CreatePresentDescriptorSet();

        void CreateCameraBuffer();
        void UpdateCameraBuffer(Camera& camera) const;
        void RotateLight(float deltaTime);

        void CreateLightsBuffer();
        void UpdateLightsBuffer();

        void BuildGBuffer();

        void CreateRenderPassResources();

        void PrepareSSAO();

        void DrawFrame(VkCommandBuffer commandBuffer, uint32_t imageIndex, Camera& camera);

    public:
        float resolutionScale = 1.0f;

        DescriptorBuffers descriptorBuffers;
        Framebuffers framebuffers;

        RenderPassResources renderPassResources;
        RenderPasses renderPasses;

        Ref<VulkanShadowMap> directionalShadowMap;
        Ref<VulkanGBuffer> gBuffer;

    private:
        VulkanDevice* device;

        VkExtent2D viewportResolution;
        VkExtent2D renderResolution;
        uint32_t activeImage = 0;

        Scene scene;
        bool isSceneLoaded = false;

        Scope<ShaderCache> shaderCache;
        Scope<VulkanSwapchain> vulkanSwapChain;

        Ref<VulkanCubeMapTexture> irradianceMap;
        float lightSpinningAngle = 0.0f;
    };
}
