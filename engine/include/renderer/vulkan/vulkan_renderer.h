#pragma once

#include "renderer/scene.h"
#include "vulkan_swapchain.h"
#include "pass/gbufferPass.h"
#include "pass/infinite_grid_pass.h"
#include "pass/lighting_pass.h"
#include "pass/present_pass.h"
#include "pass/shadow_map_pass.h"
#include "pass/skybox_pass.h"
#include "pass/lighting/brdf_lut_pass.h"
#include "pass/lighting/irradiance_map_pass.h"
#include "pass/lighting/prefilter_map_pass.h"
#include "pass/post_processing/ssao_pass.h"
#include "renderer/Light.h"
#include "renderer/shader_cache.h"

namespace MongooseVK
{

    class VulkanDevice;

    struct Framebuffers {
        Ref<VulkanFramebuffer> ssaoFramebuffer;
        Ref<VulkanFramebuffer> gbufferFramebuffer;
        Ref<VulkanFramebuffer> mainFramebuffer;
        std::vector<Ref<VulkanFramebuffer>> shadowMapFramebuffers;
        std::vector<Ref<VulkanFramebuffer>> presentFramebuffers;
        std::vector<Ref<VulkanFramebuffer>> iblIrradianceFramebuffes;
    };

    struct CameraBuffer {
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
        PassResource brdfLutTexture;
        PassResource viewspacePosition;
        PassResource viewspaceNormal;
        PassResource depthMap;
        PassResource cameraBuffer;
        PassResource lightsBuffer;
        PassResource directionalShadowMap;
        PassResource irradianceMap;
        PassResource ssaoTexture;
        PassResource mainFrameColorTexture;
        PassResource irradianceMapTexture;
        PassResource prefilterMapTexture;
    };

    struct RenderPasses {
        Scope<GBufferPass> gbufferPass;
        Scope<SkyboxPass> skyboxPass;
        Scope<LightingPass> lightingPass;
        Scope<ShadowMapPass> shadowMapPass;
        Scope<PresentPass> presentPass;
        Scope<SSAOPass> ssaoPass;
        Scope<InfiniteGridPass> gridPass;
        Scope<IrradianceMapPass> irradianceMapPass;
        Scope<BrdfLUTPass> brdfLutPass;
        Scope<PrefilterMapPass> prefilterMapPass;
    };

    class VulkanRenderer {
    public:
        VulkanRenderer() = default;
        ~VulkanRenderer();

        void Init(uint32_t width, uint32_t height);

        void LoadScene(const std::string& gltfPath, const std::string& hdrPath);

        void IdleWait();
        void Resize(int width, int height);
        void Draw(float deltaTime, Camera& camera);

        VulkanDevice* GetVulkanDevice() const { return device; }

        DirectionalLight* GetLight() { return &scene.directionalLight; }

    private:
        void CreateSwapchain();
        void CreateShadowMap();
        void CreateFramebuffers();

        void ResizeSwapchain();

        void UpdateCameraBuffer(Camera& camera) const;
        void RotateLight(float deltaTime);

        void UpdateLightsBuffer();

        void CreateRenderPassResources();
        void CreateRenderPassBuffers();
        void CreateRenderPassTextures();

        void PrecomputeIBL();
        void CalculateBrdfLUT();
        void CalculatePrefilterMap();

        void DrawFrame(VkCommandBuffer commandBuffer, uint32_t imageIndex, Camera& camera);

    public:
        VkExtent2D viewportResolution;
        VkExtent2D renderResolution;
        float resolutionScale = 1.0f;

        Framebuffers framebuffers;

        RenderPassResources renderPassResources;
        RenderPasses renderPasses;

    private:
        VulkanDevice* device;
        uint32_t activeImage = 0;

        Scene scene;
        bool isSceneLoaded = false;

        Scope<ShaderCache> shaderCache;
        Scope<VulkanSwapchain> vulkanSwapChain;

        float lightSpinningAngle = 0.0f;
    };
}
