#pragma once
#include "renderer/frame_graph/frame_graph_renderpass.h"

namespace MongooseVK
{
    struct GridParams {
        alignas(16) glm::vec3 gridColorThin = glm::vec3(0.5f, 0.5f, 0.5f);
        alignas(16) glm::vec3 gridColorThick = glm::vec3(1.0f, 1.0f, 1.0f);
        alignas(16) glm::vec3 origin = glm::vec3(0.0f, 0.001f, 0.0f);
        float gridSize = 50.0f;
        float gridCellSize = 1.0f;
    };

    class InfiniteGridPass final : public FrameGraph::FrameGraphRenderPass {
    public:
        InfiniteGridPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution);
        ~InfiniteGridPass() override = default;

        virtual void Render(VkCommandBuffer commandBuffer, SceneGraph* scene) override;

    protected:
        virtual void LoadPipeline(PipelineCreateInfo& pipelineCreate) override;

    public:
        GridParams gridParams;

    private:
        Scope<VulkanMesh> screenRect;
    };
}
