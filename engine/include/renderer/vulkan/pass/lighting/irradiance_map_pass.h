#pragma once

#include "renderer/vulkan/pass/vulkan_pass.h"

namespace MongooseVK
{
    class IrradianceMapPass final : public VulkanPass {
    public:
        explicit IrradianceMapPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution);
        ~IrradianceMapPass() override = default;

        virtual void InitFramebuffer() override;
        virtual void Render(VkCommandBuffer commandBuffer) override;
        virtual void Resize(VkExtent2D _resolution) override;

        void SetFaceIndex(uint8_t index);

    protected:
        virtual void LoadPipeline() override;

    private:
        uint8_t faceIndex = 0;
        Ref<VulkanMesh> cubeMesh;
    };
}
