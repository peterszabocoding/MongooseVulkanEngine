#pragma once

#include "renderer/scene.h"
#include "vulkan_pass.h"

namespace Raytracing
{
    class ShadowMapPass : public VulkanPass {
    public:
        explicit ShadowMapPass(VulkanDevice* vulkanDevice, Scene& _scene);
        virtual ~ShadowMapPass() override = default;

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
        Ref<VulkanPipeline> directionalShadowMapPipeline;
    };
}
