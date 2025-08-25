#pragma once

#include <map>
#include <renderer/frame_graph/frame_graph.h>

#include "renderer/scene.h"
#include "vulkan_swapchain.h"
#include "pass/lighting/brdf_lut_pass.h"
#include "pass/lighting/irradiance_map_pass.h"
#include "pass/lighting/prefilter_map_pass.h"
#include "renderer/Light.h"
#include "renderer/shader_cache.h"

namespace MongooseVK
{
    class VulkanDevice;

    struct Framebuffers {
        std::vector<FramebufferHandle> shadowMapFramebuffers;
        std::vector<FramebufferHandle> presentFramebuffers;
        std::vector<FramebufferHandle> iblIrradianceFramebuffes;
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
        alignas(4)float ambientIntensity = 0.1f;
        glm::vec4 cascadeSplits[4];
        alignas(4) float intensity = 1.0f;
        alignas(4) float bias = 0.005f;
    };

    struct SceneDefinition {
        std::string gltfPath;
        std::string hdrPath;
    };

    class VulkanRenderer {
    public:
        VulkanRenderer() = default;
        ~VulkanRenderer();

        void Init(uint32_t width, uint32_t height);
        void CalculateIBL();

        void LoadScene(const std::string& gltfPath, const std::string& hdrPath);

        void IdleWait();
        void Resize(int width, int height);
        void Draw(float deltaTime, Camera& camera);

        VulkanDevice* GetVulkanDevice() const { return device; }

        DirectionalLight* GetLight() { return &sceneGraph->directionalLight; }

        SceneGraph* GetSceneGraph() { return sceneGraph; }

    private:
        void CreateSwapchain();

        void ResizeSwapchain();
        void DrawFrame(const VkCommandBuffer& commandBuffer, uint32_t imageIndex);

        void UpdateCameraBuffer(Camera& camera);
        void RotateLight(float deltaTime);

        void UpdateLightsBuffer();
        void PresentFrame(const VkCommandBuffer& commandBuffer, uint32_t imageIndex, TextureHandle textureToPresent);

        void CreateExternalResources();

        FrameGraph::FrameGraphResource* CreateFrameGraphTextureResource(const char* resourceName, TextureCreateInfo& createInfo);
        FrameGraph::FrameGraphResource*
        CreateFrameGraphBufferResource(const char* resourceName, FrameGraph::FrameGraphBufferCreateInfo& createInfo);

        void PrecomputeIBL();

    public:
        VkExtent2D viewportResolution;
        VkExtent2D renderResolution;
        float resolutionScale = 1.0f;

        Framebuffers framebuffers;

        Scope<FrameGraph::FrameGraph> frameGraph;

        FrameGraph::FrameGraphResource* irradianceMap;
        FrameGraph::FrameGraphResource* prefilteredMap;
        FrameGraph::FrameGraphResource* brdfLUT;
        FrameGraph::FrameGraphResource* cameraBuffer;
        FrameGraph::FrameGraphResource* lightBuffer;

    private:
        VulkanDevice* device;
        uint32_t activeImage = 0;

        SceneGraph* sceneGraph;
        bool isSceneLoaded = false;

        Scope<ShaderCache> shaderCache;
        Scope<VulkanSwapchain> vulkanSwapChain;

        float lightSpinningAngle = 0.0f;

        PrefilterMapPass* prefilterPass;
        IrradianceMapPass* irradiancePass;
        BrdfLUTPass* brdfLutPass;
    };
}
