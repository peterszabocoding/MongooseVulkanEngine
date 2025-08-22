#pragma once
#include <renderer/frame_graph.h>

namespace MongooseVK
{
    class IrradianceMapPass final : public FrameGraphRenderPass {
    public:
        explicit IrradianceMapPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution);
        ~IrradianceMapPass() override = default;

        virtual void CreateFramebuffer() override;
        virtual void Render(VkCommandBuffer commandBuffer, SceneGraph* scene) override;
        virtual void Resize(VkExtent2D _resolution) override;

        void SetFaceIndex(uint8_t index);
        void SetCubemapTexture(TextureHandle cubemapTexture);

    protected:
        virtual void LoadPipeline(PipelineCreateInfo& pipelineCreate) override;

    private:
        uint8_t faceIndex = 0;
        Ref<VulkanMesh> cubeMesh;
        TextureHandle cubemapTexture = INVALID_TEXTURE_HANDLE;
    };
}
