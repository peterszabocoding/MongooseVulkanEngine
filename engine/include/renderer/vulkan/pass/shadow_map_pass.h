#pragma once

#include "renderer/frame_graph/frame_graph.h"

#include "renderer/scene.h"

#include "renderer/frame_graph/frame_graph_renderpass.h"

namespace MongooseVK
{
    constexpr uint32_t SHADOW_MAP_RESOLUTION = 4096;

    class ShadowMapPass final : public FrameGraph::FrameGraphRenderPass {
    public:
        explicit ShadowMapPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution);
        ~ShadowMapPass() override = default;

        void SetCascadeIndex(uint32_t _cascadeIndex) { cascadeIndex = _cascadeIndex; }

        virtual void Init() override;
        virtual void CreateFramebuffer() override;
        virtual void Render(VkCommandBuffer commandBuffer, SceneGraph* scene) override;
        virtual void Resize(VkExtent2D _resolution) override;

    protected:
        virtual void LoadPipeline(PipelineCreateInfo& pipelineCreate) override;

    private:
        uint32_t cascadeIndex = 0;
    };
}
