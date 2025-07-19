#pragma once

#include "renderer/vulkan/pass/vulkan_pass.h"

namespace MongooseVK
{
    class IrradianceMapPass final : public VulkanPass {
    public:
        explicit IrradianceMapPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution);
        ~IrradianceMapPass() override = default;

        virtual void Render(VkCommandBuffer commandBuffer, Camera* camera, FramebufferHandle writeBuffer) override;
        virtual void Resize(VkExtent2D _resolution) override;

        void SetFaceIndex(uint8_t index);

    private:
        void LoadPipeline();

    private:
        uint8_t faceIndex = 0;
        Ref<VulkanMesh> cubeMesh;
    };
}
