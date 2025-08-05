#pragma once
#include <renderer/frame_graph.h>

namespace MongooseVK
{
    class UiPass final : public FrameGraphRenderPass {
    public:
        explicit UiPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution);
        ~UiPass() override = default;

        virtual void Render(VkCommandBuffer commandBuffer) override;

    protected:
        virtual void LoadPipeline() override;
    };
}
