#pragma once

#include "renderer/scene.h"
#include "vulkan_pass.h"

namespace Raytracing
{
    class ShadowMapPass : public VulkanPass {
    public:
        explicit ShadowMapPass(VulkanDevice* vulkanDevice, Scene& _scene, VkExtent2D _resolution);
        virtual ~ShadowMapPass() override = default;

        void SetCascadeIndex(uint32_t _cascadeIndex) { cascadeIndex = _cascadeIndex; }

        virtual void Render(VkCommandBuffer commandBuffer,
                            Camera& camera,
                            Ref<VulkanFramebuffer> writeBuffer,
                            Ref<VulkanFramebuffer> readBuffer = nullptr) override;

        Ref<VulkanRenderPass> GetRenderPass() { return renderPass; }

    private:
        void LoadPipelines();

    private:
        Scene& scene;
        uint32_t cascadeIndex = 0;
        Ref<VulkanRenderPass> renderPass{};
        Ref<VulkanPipeline> directionalShadowMapPipeline;
    };
}
