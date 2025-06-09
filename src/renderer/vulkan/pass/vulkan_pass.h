#pragma once
#include "../vulkan_device.h"
#include "../vulkan_framebuffer.h"
#include "../../../util/core.h"

namespace Raytracing
{
    class VulkanPass {

    public:
        struct Config {
            VkExtent2D imageExtent;
        };

    public:
        explicit VulkanPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution): device(vulkanDevice), resolution(_resolution) {}
        virtual ~VulkanPass() = default;

        virtual void Render(VkCommandBuffer commandBuffer,
                            Camera& camera,
                            Ref<VulkanFramebuffer> writeBuffer,
                            Ref<VulkanFramebuffer> readBuffer = nullptr) = 0;

        virtual void Resize(VkExtent2D _resolution)
        {
            resolution = _resolution;
        }

        virtual void OnResolutionChanged(const uint32_t width, const uint32_t height) {}

    protected:
        VulkanDevice* device;
        VkExtent2D resolution;
    };
}
