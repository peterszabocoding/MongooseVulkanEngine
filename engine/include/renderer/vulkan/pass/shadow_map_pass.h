#pragma once

#include <renderer/frame_graph.h>

#include "renderer/scene.h"

namespace MongooseVK
{
    constexpr uint32_t SHADOW_MAP_RESOLUTION = 4096;

    class ShadowMapPass final : public FrameGraphRenderPass {
    public:
        explicit ShadowMapPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution);
        ~ShadowMapPass() override = default;

        void SetCascadeIndex(uint32_t _cascadeIndex) { cascadeIndex = _cascadeIndex; }

        virtual void Init() override;
        virtual void CreateFramebuffer() override;
        virtual void Render(VkCommandBuffer commandBuffer, Scene* scene) override;

    protected:
        virtual void LoadPipeline() override;

    private:
        uint32_t cascadeIndex = 0;
    };
}
