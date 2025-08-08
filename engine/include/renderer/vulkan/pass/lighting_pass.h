#pragma once

#include <renderer/frame_graph.h>

#include "renderer/scene.h"

namespace MongooseVK
{
    class LightingPass final : public FrameGraphRenderPass {
    public:
        explicit LightingPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution);
        ~LightingPass() override = default;

        virtual void Render(VkCommandBuffer commandBuffer, Scene* scene) override;

    protected:
        virtual void LoadPipeline(PipelineCreateInfo& pipelineCreate) override;
    };
}
