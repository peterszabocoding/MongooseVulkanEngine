#pragma once

#include "renderer/scene.h"
#include "vulkan_pass.h"

namespace Raytracing
{
    class RenderPass : public VulkanPass {
    public:
        explicit RenderPass(VulkanDevice* vulkanDevice, Scene& _scene);
        virtual ~RenderPass() override = default;

        virtual void Render(VkCommandBuffer commandBuffer,
                            Camera& camera,
                            Ref<VulkanFramebuffer> writeBuffer,
                            Ref<VulkanFramebuffer> readBuffer = nullptr) override;
        void DrawSkybox(VkCommandBuffer commandBuffer) const;

        Ref<VulkanRenderPass> GetRenderPass() { return renderPass; }

    private:
        void LoadPipelines();

    private:
        Scene& scene;

        Ref<VulkanRenderPass> renderPass{};
        Ref<VulkanPipeline> geometryPipeline;
    };
}
