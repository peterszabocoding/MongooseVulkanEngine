#pragma once
#include "vulkan_pass.h"
#include "renderer/scene.h"

namespace MongooseVK
{
    class SkyboxPass final : public VulkanPass {
    public:
        SkyboxPass(VulkanDevice* vulkanDevice, Scene& _scene, VkExtent2D _resolution);
        ~SkyboxPass() override = default;

        virtual void Render(VkCommandBuffer commandBuffer, Camera* camera, FramebufferHandle writeBuffer) override;

    protected:
        virtual void LoadPipeline() override;

    private:
        Scene& scene;
        Ref<VulkanMesh> cubeMesh;
    };
}
