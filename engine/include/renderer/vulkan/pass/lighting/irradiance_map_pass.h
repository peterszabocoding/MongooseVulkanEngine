#pragma once
#include "renderer/frame_graph/frame_graph_renderpass.h"

namespace MongooseVK
{
    class IrradianceMapPass final : public FrameGraph::FrameGraphRenderPass {
    public:
        explicit IrradianceMapPass(VulkanDevice* vulkanDevice);
        ~IrradianceMapPass() override = default;

        virtual void CreateFramebuffer() override;
        virtual void Render(VkCommandBuffer commandBuffer, SceneGraph* scene) override;

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
