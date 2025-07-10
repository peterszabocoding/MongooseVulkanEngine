#pragma once

#include "renderer/scene.h"
#include "vulkan_pass.h"

namespace MongooseVK
{
    class RenderPass : public VulkanPass {
    public:
        explicit RenderPass(VulkanDevice* vulkanDevice, Scene& _scene, VkExtent2D _resolution);
        virtual ~RenderPass() override;

        virtual void Render(VkCommandBuffer commandBuffer,
                            Camera& camera,
                            Ref<VulkanFramebuffer> writeBuffer,
                            Ref<VulkanFramebuffer> readBuffer = nullptr) override;
        void DrawSkybox(VkCommandBuffer commandBuffer) const;

        VulkanRenderPass* GetRenderPass();

    private:
        void LoadPipelines();

    private:
        Scene& scene;
        RenderPassHandle renderPassHandle;
        Ref<VulkanPipeline> geometryPipeline;
    };
}
