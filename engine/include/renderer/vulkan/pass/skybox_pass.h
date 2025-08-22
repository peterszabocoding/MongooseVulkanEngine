#pragma once
#include <renderer/frame_graph.h>

#include "renderer/scene.h"

namespace MongooseVK
{
    class SkyboxPass final : public FrameGraphRenderPass {
    public:
        SkyboxPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution);
        ~SkyboxPass() override = default;

        virtual void Render(VkCommandBuffer commandBuffer, SceneGraph* scene) override;

    protected:
        virtual void LoadPipeline(PipelineCreateInfo& pipelineCreate) override;

    private:
        Ref<VulkanMesh> cubeMesh;
    };
}
