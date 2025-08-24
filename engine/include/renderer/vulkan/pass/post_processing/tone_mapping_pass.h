#pragma once
#include <renderer/graph/frame_graph.h>
#include <renderer/graph/frame_graph_renderpass.h>

#include "renderer/vulkan/vulkan_mesh.h"

namespace MongooseVK
{
    class ToneMappingPass final : public FrameGraphRenderPass {
    public:
        struct ToneMappingParams {
            float exposure = 1.0;
            float gamma = 2.2;
        };

    public:
        explicit ToneMappingPass(VulkanDevice* _device, VkExtent2D _resolution);
        ~ToneMappingPass() override;

        virtual void Setup(FrameGraph* frameGraph) override;
        virtual void Render(VkCommandBuffer commandBuffer, SceneGraph* scene) override;
        virtual void Resize(VkExtent2D _resolution) override;

    protected:
        virtual void LoadPipeline(PipelineCreateInfo& pipelineCreate) override;

    public:
        ToneMappingParams toneMappingParams{};

    private:
        Scope<VulkanMesh> screenRect;
    };
};
