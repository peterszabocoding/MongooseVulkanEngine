#pragma once
#include "vulkan_pass.h"
#include "renderer/scene.h"

namespace MongooseVK
{
    class SkyboxPass : public VulkanPass {
    public:
        SkyboxPass(VulkanDevice* vulkanDevice, Scene& _scene, VkExtent2D _resolution);
        ~SkyboxPass() override = default;

        virtual void Render(VkCommandBuffer commandBuffer, Camera* camera, FramebufferHandle writeBuffer) override;

    private:
        void LoadPipelines();

    private:
        Scene& scene;

        Ref<VulkanPipeline> skyboxPipeline;
        Ref<VulkanMesh> cubeMesh;

        TextureHandle outputTexture;

        Ref<VulkanFramebuffer> framebuffer;
    };
} // Raytracing
