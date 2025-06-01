#pragma once

#include "vulkan_pass.h"

namespace Raytracing
{
    class PresentPass : public VulkanPass {
    public:
        explicit PresentPass(VulkanDevice* vulkanDevice);
        virtual ~PresentPass() override = default;

        virtual void Render(VkCommandBuffer commandBuffer,
                            Camera& camera,
                            uint32_t imageIndex,
                            Ref<VulkanFramebuffer> writeBuffer,
                            Ref<VulkanFramebuffer> readBuffer = nullptr) override;

        Ref<VulkanRenderPass> GetRenderPass() { return renderPass; }

    private:
        void LoadPipeline();

    private:
        Ref<VulkanRenderPass> renderPass{};
        Scope<VulkanMeshlet> screenRect;
        Ref<VulkanPipeline> presentPipeline;
    };
}
