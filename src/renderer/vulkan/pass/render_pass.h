#pragma once

#include "renderer/scene.h"
#include "vulkan_pass.h"

namespace Raytracing
{
    class RenderPass : public VulkanPass {
    public:
        explicit RenderPass(VulkanDevice* vulkanDevice, Scene& _scene);
        virtual ~RenderPass() override = default;

        void SetCamera(const Camera& camera);

        virtual void Render(VkCommandBuffer commandBuffer,
                            uint32_t imageIndex,
                            Ref<VulkanFramebuffer> writeBuffer,
                            Ref<VulkanFramebuffer> readBuffer) override;
        void DrawSkybox(VkCommandBuffer commandBuffer) const;

        Ref<VulkanRenderPass> GetRenderPass() { return renderPass; }

    private:
        void LoadPipelines();

    private:
        Scene& scene;
        Camera camera;

        Ref<VulkanRenderPass> renderPass{};
        Ref<VulkanMesh> cubeMesh;

        Ref<VulkanPipeline> skyBoxPipeline;
        Ref<VulkanPipeline> geometryPipeline;
    };
}
