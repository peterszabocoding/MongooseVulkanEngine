#pragma once
#include "renderer/frame_graph/frame_graph_renderpass.h"

namespace MongooseVK
{
    class UiPass final : public FrameGraph::FrameGraphRenderPass {
    public:
        explicit UiPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution);
        ~UiPass() override = default;

        virtual void Render(VkCommandBuffer commandBuffer, SceneGraph* scene) override;

    protected:
        virtual void LoadPipeline(PipelineCreateInfo& pipelineCreate) override;
    };
}
