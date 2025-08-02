#pragma once

#include "vulkan_pass.h"

namespace MongooseVK
{
    class UiPass final : public VulkanPass {
    public:
        explicit UiPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution);
        ~UiPass() override = default;

        virtual void Render(VkCommandBuffer commandBuffer) override;

    protected:
        virtual void LoadPipeline() override;
    };
}