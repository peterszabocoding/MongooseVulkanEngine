#pragma once
#include <renderer/graph/frame_graph.h>
#include <renderer/graph/frame_graph_renderpass.h>

namespace MongooseVK
{
    class UiPass final : public FrameGraphRenderPass {
    public:
        explicit UiPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution);
        ~UiPass() override = default;

        virtual void Setup(FrameGraph* frameGraph) override;
        virtual void Render(VkCommandBuffer commandBuffer, Scene* scene) override;

    protected:
        virtual void LoadPipeline(PipelineCreateInfo& pipelineCreate) override;
    };
}
