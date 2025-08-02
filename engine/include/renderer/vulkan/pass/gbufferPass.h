#pragma once
#include "vulkan_pass.h"
#include "renderer/scene.h"

namespace MongooseVK

{
    class GBufferPass final : public VulkanPass {
    public:
        explicit GBufferPass(VulkanDevice* vulkanDevice, Scene& _scene, VkExtent2D _resolution);
        ~GBufferPass() override = default;

        virtual void Init() override;
        virtual void Render(VkCommandBuffer commandBuffer) override;
        virtual void Resize(VkExtent2D _resolution) override;

    protected:
        virtual void LoadPipeline() override;

    private:
        Scene& scene;
    };
}
