#pragma once

#include "vulkan_pass.h"

namespace MongooseVK
{
    class PresentPass : public VulkanPass {
    public:
        explicit PresentPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution);
        virtual ~PresentPass() override;

        virtual void Render(VkCommandBuffer commandBuffer,
                            Camera& camera,
                            Ref<VulkanFramebuffer> writeBuffer,
                            Ref<VulkanFramebuffer> readBuffer = nullptr) override;

        VulkanRenderPass* GetRenderPass();

    private:
        void LoadPipeline();

    private:
        RenderPassHandle renderPassHandle;
        Scope<VulkanMeshlet> screenRect;
        Ref<VulkanPipeline> presentPipeline;
    };
}
