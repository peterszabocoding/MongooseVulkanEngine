#pragma once
#include "vulkan_pass.h"
#include "renderer/scene.h"

namespace MongooseVK

{
    class GBufferPass final : public VulkanPass {
    public:
        explicit GBufferPass(VulkanDevice* vulkanDevice, Scene& _scene, VkExtent2D _resolution);
        ~GBufferPass() override = default;

        virtual void Render(VkCommandBuffer commandBuffer, Camera* camera, FramebufferHandle writeBuffer) override;
        virtual void Resize(VkExtent2D _resolution) override;

    private:
        void LoadPipelines();

    private:
        Scene& scene;
        Ref<VulkanPipeline> gbufferPipeline;
    };
}
