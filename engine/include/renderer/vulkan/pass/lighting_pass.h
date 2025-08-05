#pragma once

#include <renderer/frame_graph.h>

#include "renderer/scene.h"

namespace MongooseVK
{
    class LightingPass final : public FrameGraphRenderPass {
    public:
        explicit LightingPass(VulkanDevice* vulkanDevice, Scene& _scene, VkExtent2D _resolution);
        ~LightingPass() override = default;

        virtual void Render(VkCommandBuffer commandBuffer) override;

    protected:
        virtual void LoadPipeline() override;

    private:
        Scene& scene;
    };
}
