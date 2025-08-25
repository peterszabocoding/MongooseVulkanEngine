#pragma once

#include "renderer/frame_graph/frame_graph_renderpass.h"

#include "renderer/scene.h"

namespace MongooseVK

{
    class GBufferPass final : public FrameGraph::FrameGraphRenderPass {
    public:
        explicit GBufferPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution);
        ~GBufferPass() override = default;

        virtual void Render(VkCommandBuffer commandBuffer, SceneGraph* scene) override;

    protected:
        virtual void LoadPipeline(PipelineCreateInfo& pipelineCreate) override;
    };
}
