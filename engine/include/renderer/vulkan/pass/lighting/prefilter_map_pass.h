#pragma once
#include "renderer/frame_graph/frame_graph_renderpass.h"

namespace MongooseVK
{
    class PrefilterMapPass final: public FrameGraph::FrameGraphRenderPass {
    public:
        PrefilterMapPass(VulkanDevice* vulkanDevice);

        virtual void CreateFramebuffer() override;
        virtual void Render(VkCommandBuffer commandBuffer, SceneGraph* scene) override;

        void SetRoughness(float _roughness);
        void SetCubemapTexture(TextureHandle _cubemapTextureHandle);
        void SetTargetTexture(TextureHandle _targetTexture);

    protected:
        virtual void LoadPipeline(PipelineCreateInfo& pipelineCreate) override;

    private:
        Ref<VulkanMesh> cubeMesh;

        float roughness = 0.0f;
        TextureHandle cubemapTextureHandle = INVALID_TEXTURE_HANDLE;
        TextureHandle targetTexture = INVALID_TEXTURE_HANDLE;
    };
}
