#pragma once

#include "renderer/frame_graph/frame_graph_renderpass.h"
#include "renderer/scene.h"

namespace MongooseVK
{
    class LightingPass final : public FrameGraph::FrameGraphRenderPass {
    public:
        explicit LightingPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution);
        ~LightingPass() override = default;

        virtual void Render(VkCommandBuffer commandBuffer, SceneGraph* scene) override;

    protected:
        virtual void LoadPipeline(PipelineCreateInfo& pipelineCreate) override;
    };
}
