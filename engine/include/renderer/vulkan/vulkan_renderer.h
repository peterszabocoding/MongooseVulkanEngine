#pragma once

#include <renderer/graph/frame_graph.h>

#include "renderer/scene.h"
#include "vulkan_swapchain.h"
#include "pass/post_processing/tone_mapping_pass.h"
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
        void InitializeRenderPasses();

        void LoadScene(const std::string& gltfPath, const std::string& hdrPath);

        void IdleWait();
        void Resize(int width, int height);
        void Draw(float deltaTime, Camera& camera);

        VulkanDevice* GetVulkanDevice() const { return device; }

        DirectionalLight* GetLight() { return &scene.directionalLight; }

    private:
        void CreateSwapchain();

        void ResizeSwapchain();
        void DrawFrame(const VkCommandBuffer& commandBuffer, uint32_t imageIndex);

        void UpdateCameraBuffer(Camera& camera);
        void RotateLight(float deltaTime);

        void UpdateLightsBuffer();
        void PresentFrame(const VkCommandBuffer& commandBuffer, uint32_t imageIndex);

        void CreateFrameGraphOutputs();
        void CreateFrameGraphInputs();
        void DestroyResource(const char* resourceName);
        void CreateFrameGraphResource(const char* resourceName, FrameGraphResourceType type, FrameGraphResourceCreateInfo& createInfo);

        void PrecomputeIBL();

        template<typename T>
        void AddRenderPass(const char* name)
        {
            frameGraphRenderPasses[name] = new T(device, renderResolution);
        }

    public:
        VkExtent2D viewportResolution;
        VkExtent2D renderResolution;
        float resolutionScale = 1.0f;

        Framebuffers framebuffers;

        ObjectResourcePool<FrameGraphResource> frameGraphResources;
        std::unordered_map<std::string, FrameGraphResourceHandle> frameGraphResourceHandles;
        std::unordered_map<std::string, FrameGraphResource> renderPassResourceMap;
        std::unordered_map<std::string, FrameGraphRenderPass*> frameGraphRenderPasses;

        std::unordered_map<std::string, FrameGraphResourceCreate> frameGraphOutputCreations;
        std::unordered_map<std::string, FrameGraphResourceCreate> frameGraphInputCreations;

        Scope<FrameGraph> frameGraph;

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
