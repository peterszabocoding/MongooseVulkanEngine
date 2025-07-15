#pragma once
#include <renderer/vulkan/pass/vulkan_pass.h>

namespace MongooseVK
{
    class PrefilterMapPass : public VulkanPass {
    public:
        PrefilterMapPass(VulkanDevice* vulkanDevice, const VkExtent2D& _resolution);

        virtual void Render(VkCommandBuffer commandBuffer, Camera* camera, Ref<VulkanFramebuffer> writeBuffer,
                            Ref<VulkanFramebuffer> readBuffer) override;

        virtual void Resize(VkExtent2D _resolution) override;

        void SetCubemapResolution(float _cubemapResolution);
        void SetTargetTexture(TextureHandle _targetTexture);

    private:
        void LoadPipeline();

    private:
        Ref<VulkanMesh> cubeMesh;
        Ref<VulkanPipeline> prefilterMapPipeline;

        uint32_t cubemapResolution = 0;
        TextureHandle targetTexture;
    };
}
