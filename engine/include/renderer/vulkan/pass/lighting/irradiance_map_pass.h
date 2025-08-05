#pragma once
#include <renderer/frame_graph.h>

namespace MongooseVK
{
    class IrradianceMapPass final : public FrameGraphRenderPass {
    public:
        explicit IrradianceMapPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution);
        ~IrradianceMapPass() override = default;

        virtual void CreateFramebuffer() override;
        virtual void Render(VkCommandBuffer commandBuffer, Scene* scene) override;
        virtual void Resize(VkExtent2D _resolution) override;

        void SetFaceIndex(uint8_t index);

    protected:
        virtual void LoadPipeline() override;

    private:
        uint8_t faceIndex = 0;
        Ref<VulkanMesh> cubeMesh;
    };
}
