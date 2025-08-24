#pragma once

#include <renderer/graph/frame_graph.h>
#include <renderer/graph/frame_graph_renderpass.h>

#include "renderer/scene.h"

namespace MongooseVK
{
    class LightingPass final : public FrameGraphRenderPass {
    public:
        explicit LightingPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution);
        ~LightingPass() override = default;

        virtual void Setup(FrameGraph* frameGraph) override;
        virtual void Render(VkCommandBuffer commandBuffer, SceneGraph* scene) override;

    protected:
        virtual void LoadPipeline(PipelineCreateInfo& pipelineCreate) override;
    };
}
