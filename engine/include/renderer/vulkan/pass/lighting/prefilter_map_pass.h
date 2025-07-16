#pragma once
#include <renderer/vulkan/pass/vulkan_pass.h>

namespace MongooseVK
{
    class PrefilterMapPass: public VulkanPass {
    public:
        PrefilterMapPass(VulkanDevice* vulkanDevice, const VkExtent2D& _resolution);

        virtual void Render(VkCommandBuffer commandBuffer, Camera* camera, FramebufferHandle writeBuffer) override;

        virtual void Resize(VkExtent2D _resolution) override;

        void SetRoughness(float _roughness);
        void SetCubemapTexture(TextureHandle _cubemapTextureHandle);
        void SetTargetTexture(TextureHandle _targetTexture);

    private:
        void LoadPipeline();

    private:
        Ref<VulkanMesh> cubeMesh;
        Ref<VulkanPipeline> prefilterMapPipeline;

        float roughness = 0.0f;
        TextureHandle cubemapTextureHandle;
        TextureHandle targetTexture;
    };
}
