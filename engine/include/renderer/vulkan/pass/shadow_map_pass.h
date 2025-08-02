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

        virtual void Init() override;
        virtual void InitFramebuffer() override;
        virtual void Render(VkCommandBuffer commandBuffer) override;

    protected:
        virtual void LoadPipeline() override;

    private:
        Scene& scene;
        uint32_t cascadeIndex = 0;
    };
}
