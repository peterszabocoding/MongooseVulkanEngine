#pragma once

#include "vulkan_pass.h"

namespace MongooseVK
{
    class PresentPass final : public VulkanPass {
    public:
        explicit PresentPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution);
        ~PresentPass() override = default;

        virtual void Render(VkCommandBuffer commandBuffer, Camera* camera, FramebufferHandle writeBuffer) override;

    protected:
        virtual void LoadPipeline() override;

    private:
        Scope<VulkanMeshlet> screenRect;
        Scope<VulkanFramebuffer> framebuffer;
    };
}
