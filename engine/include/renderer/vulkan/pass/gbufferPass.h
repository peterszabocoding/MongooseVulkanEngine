#pragma once

#include <renderer/graph/frame_graph.h>
#include <renderer/graph/frame_graph_renderpass.h>

#include "renderer/scene.h"

namespace MongooseVK

{
    class GBufferPass final : public FrameGraphRenderPass {
    public:
        explicit GBufferPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution);
        ~GBufferPass() override = default;

        virtual void Setup(FrameGraph* frameGraph) override;
        virtual void Init() override;
        virtual void Render(VkCommandBuffer commandBuffer, SceneGraph* scene) override;
        virtual void Resize(VkExtent2D _resolution) override;

    protected:
        virtual void LoadPipeline(PipelineCreateInfo& pipelineCreate) override;
    };
}
