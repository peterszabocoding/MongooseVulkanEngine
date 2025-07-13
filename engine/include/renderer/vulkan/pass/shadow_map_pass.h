#pragma once

#include "renderer/scene.h"
#include "vulkan_pass.h"

namespace MongooseVK
{
    class ShadowMapPass final : public VulkanPass {
    public:
        explicit ShadowMapPass(VulkanDevice* vulkanDevice, Scene& _scene, VkExtent2D _resolution);
        ~ShadowMapPass() override = default;

        void SetCascadeIndex(uint32_t _cascadeIndex) { cascadeIndex = _cascadeIndex; }

        virtual void Render(VkCommandBuffer commandBuffer,
                            Camera* camera,
                            Ref<VulkanFramebuffer> writeBuffer,
                            Ref<VulkanFramebuffer> readBuffer = nullptr) override;

    private:
        void LoadPipelines();

    private:
        Scene& scene;
        uint32_t cascadeIndex = 0;
        Ref<VulkanPipeline> directionalShadowMapPipeline;
    };
}
