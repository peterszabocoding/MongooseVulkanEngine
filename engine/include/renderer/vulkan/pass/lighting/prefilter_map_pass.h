#pragma once
#include <renderer/vulkan/pass/vulkan_pass.h>

namespace MongooseVK
{
    class PrefilterMapPass final: public VulkanPass {
    public:
        PrefilterMapPass(VulkanDevice* vulkanDevice, const VkExtent2D& _resolution);

        virtual void Init() override;
        virtual void InitFramebuffer() override;
        virtual void Render(VkCommandBuffer commandBuffer) override;
        virtual void Resize(VkExtent2D _resolution) override;

        void SetRoughness(float _roughness);
        void SetCubemapTexture(TextureHandle _cubemapTextureHandle);
        void SetTargetTexture(TextureHandle _targetTexture);

    protected:
        virtual void LoadPipeline() override;

    private:
        Ref<VulkanMesh> cubeMesh;

        float roughness = 0.0f;
        TextureHandle cubemapTextureHandle;
        TextureHandle targetTexture;
    };
}
