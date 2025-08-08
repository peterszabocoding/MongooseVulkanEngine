#pragma once

#include <renderer/frame_graph.h>

#include "renderer/scene.h"

namespace MongooseVK

{
    class GBufferPass final : public FrameGraphRenderPass {
    public:
        explicit GBufferPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution);
        ~GBufferPass() override = default;

        virtual void Init() override;
        virtual void Render(VkCommandBuffer commandBuffer, Scene* scene) override;
        virtual void Resize(VkExtent2D _resolution) override;

    protected:
        virtual void LoadPipeline(PipelineCreateInfo& pipelineCreate) override;
    };
}
