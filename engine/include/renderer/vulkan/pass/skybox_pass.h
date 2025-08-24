#pragma once
#include <renderer/graph/frame_graph.h>
#include <renderer/graph/frame_graph_renderpass.h>

#include "renderer/scene.h"

namespace MongooseVK
{
    class SkyboxPass final : public FrameGraphRenderPass {
    public:
        SkyboxPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution);
        ~SkyboxPass() override = default;

        virtual void Setup(FrameGraph* frameGraph) override;
        virtual void Render(VkCommandBuffer commandBuffer, SceneGraph* scene) override;

    protected:
        virtual void LoadPipeline(PipelineCreateInfo& pipelineCreate) override;

    private:
        Ref<VulkanMesh> cubeMesh;
    };
}
