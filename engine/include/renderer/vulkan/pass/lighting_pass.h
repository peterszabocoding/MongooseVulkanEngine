#pragma once

#include "renderer/scene.h"
#include "vulkan_pass.h"

namespace MongooseVK
{
    class LightingPass final : public VulkanPass {
    public:
        explicit LightingPass(VulkanDevice* vulkanDevice, Scene& _scene, VkExtent2D _resolution);
        ~LightingPass() override = default;

        virtual void Render(VkCommandBuffer commandBuffer, Camera* camera, FramebufferHandle writeBuffer) override;
        void DrawSkybox(VkCommandBuffer commandBuffer) const;

    protected:
        virtual void LoadPipeline() override;

    private:
        Scene& scene;
    };
}
