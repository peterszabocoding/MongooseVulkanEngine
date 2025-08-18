#pragma once
#include <renderer/graph/frame_graph.h>
#include <renderer/graph/frame_graph_renderpass.h>

namespace MongooseVK
{
    class PrefilterMapPass final: public FrameGraphRenderPass {
    public:
        PrefilterMapPass(VulkanDevice* vulkanDevice, const VkExtent2D& _resolution);

        virtual void Init() override;
        virtual void CreateFramebuffer() override;
        virtual void Render(VkCommandBuffer commandBuffer, Scene* scene) override;
        virtual void Resize(VkExtent2D _resolution) override;

        void SetRoughness(float _roughness);
        void SetCubemapTexture(TextureHandle _cubemapTextureHandle);
        void SetTargetTexture(TextureHandle _targetTexture);

    protected:
        virtual void LoadPipeline(PipelineCreateInfo& pipelineCreate) override;

    private:
        Ref<VulkanMesh> cubeMesh;

        float roughness = 0.0f;
        TextureHandle cubemapTextureHandle;
        TextureHandle targetTexture;
    };
}
