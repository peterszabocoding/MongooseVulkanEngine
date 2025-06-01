#pragma once
#include "vulkan_pass.h"
#include "renderer/scene.h"

namespace Raytracing

{
    class GBufferPass : public VulkanPass {
    public:
        explicit GBufferPass(VulkanDevice* vulkanDevice, Scene& _scene);
        virtual ~GBufferPass() override = default;

        virtual void Render(VkCommandBuffer commandBuffer,
                            Camera& camera,
                            uint32_t imageIndex,
                            Ref<VulkanFramebuffer> writeBuffer,
                            Ref<VulkanFramebuffer> readBuffer = nullptr) override;

        Ref<VulkanRenderPass> GetRenderPass() { return renderPass; }

    private:
        void LoadPipelines();

    private:
        Scene& scene;

        Ref<VulkanRenderPass> renderPass{};
        Ref<VulkanPipeline> gbufferPipeline;
    };
}
