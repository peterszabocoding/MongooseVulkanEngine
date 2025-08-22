#pragma once
#include <renderer/frame_graph.h>
#include <renderer/vulkan/vulkan_mesh.h>

namespace MongooseVK
{
    class BrdfLUTPass final : public FrameGraphRenderPass {
    public:
        BrdfLUTPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution);
        ~BrdfLUTPass() override = default;

        virtual void Init() override;
        virtual void Render(VkCommandBuffer commandBuffer, SceneGraph* scene) override;
        virtual void Resize(VkExtent2D _resolution) override;

    protected:
        virtual void LoadPipeline(PipelineCreateInfo& pipelineCreate) override;

    private:
        Ref<VulkanMesh> screenRect;
    };
}
