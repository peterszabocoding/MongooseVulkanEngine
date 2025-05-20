#pragma once
#include "vulkan_pass.h"
#include "renderer/scene.h"

namespace Raytracing

{
    class GBufferPass : public VulkanPass {
    public:
        explicit GBufferPass(VulkanDevice* vulkanDevice, Scene& _scene);
        virtual ~GBufferPass() override = default;

        void SetCamera(const Camera& camera);

        virtual void Render(VkCommandBuffer commandBuffer,
                            uint32_t imageIndex,
                            Ref<VulkanFramebuffer> writeBuffer,
                            Ref<VulkanFramebuffer> readBuffer) override;

        Ref<VulkanRenderPass> GetRenderPass() { return renderPass; }

    private:
        void LoadPipelines();

    private:
        Scene& scene;
        Camera camera;

        Ref<VulkanRenderPass> renderPass{};
        Ref<VulkanPipeline> gbufferPipeline;
    };
}
