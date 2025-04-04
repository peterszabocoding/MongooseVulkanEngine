#pragma once

#include <vulkan/vulkan_core.h>

namespace Raytracing {
    class VulkanDevice;

    class VulkanRenderPass {
    public:
        VulkanRenderPass(VulkanDevice* device, VkFormat imageFormat);

        ~VulkanRenderPass();

        [[nodiscard]] VkRenderPass Get() const { return renderPass; }

    private:
        void CreateRenderPass(VkFormat imageFormat);

    private:
        VulkanDevice* device;
        VkRenderPass renderPass{};
    };
}
