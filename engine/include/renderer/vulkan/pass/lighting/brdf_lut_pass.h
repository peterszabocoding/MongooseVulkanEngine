#pragma once
#include "renderer/frame_graph/frame_graph_renderpass.h"
#include <renderer/vulkan/vulkan_mesh.h>

namespace MongooseVK
{
    class BrdfLUTPass final : public FrameGraph::FrameGraphRenderPass {
    public:
        BrdfLUTPass(VulkanDevice* vulkanDevice);
        ~BrdfLUTPass() override = default;

        virtual void Render(VkCommandBuffer commandBuffer, SceneGraph* scene) override;

    protected:
        virtual void LoadPipeline(PipelineCreateInfo& pipelineCreate) override;

    private:
        Scope<VulkanMesh> screenRect;
    };
}
