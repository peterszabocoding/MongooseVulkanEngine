#pragma once

#include "vulkan_pass.h"

namespace MongooseVK
{
    class PresentPass final : public VulkanPass {
    public:
        explicit PresentPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution);
        ~PresentPass() override = default;

        virtual void Render(VkCommandBuffer commandBuffer,
                            Camera& camera,
                            Ref<VulkanFramebuffer> writeBuffer,
                            Ref<VulkanFramebuffer> readBuffer = nullptr) override;

    private:
        void CreateFramebuffer();
        void LoadPipeline();

    private:
        Scope<VulkanMeshlet> screenRect;
        Ref<VulkanPipeline> presentPipeline;
        Scope<VulkanFramebuffer> framebuffer;
    };
}
