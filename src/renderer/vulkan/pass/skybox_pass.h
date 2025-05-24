#pragma once
#include "vulkan_pass.h"
#include "renderer/scene.h"

namespace Raytracing {

    class SkyboxPass : public VulkanPass {
    public:
        SkyboxPass(VulkanDevice* vulkanDevice, Scene& _scene);
        ~SkyboxPass() override = default;

        virtual void Render(VkCommandBuffer commandBuffer,
                            uint32_t imageIndex,
                            Ref<VulkanFramebuffer> writeBuffer,
                            Ref<VulkanFramebuffer> readBuffer) override;

        Ref<VulkanRenderPass> GetRenderPass() { return renderPass; }

    private:
        void LoadPipelines();

    private:
        Scene& scene;

        Ref<VulkanRenderPass> renderPass{};
        Ref<VulkanPipeline> skyboxPipeline;
        Ref<VulkanMesh> cubeMesh;
    };

} // Raytracing
