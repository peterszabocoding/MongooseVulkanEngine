#pragma once
#include "../vulkan_device.h"
#include "../vulkan_framebuffer.h"
#include "../../../util/core.h"

namespace Raytracing
{
    class VulkanPass {
    public:
        explicit VulkanPass(VulkanDevice* vulkanDevice): device{vulkanDevice} {}
        virtual ~VulkanPass() = default;

        virtual void Render(VkCommandBuffer commandBuffer,
                            uint32_t imageIndex,
                            Ref<VulkanFramebuffer> writeBuffer,
                            Ref<VulkanFramebuffer> readBuffer) = 0;

        void SetSize(const uint32_t width, const uint32_t height)
        {
            ASSERT(width <= 0 || height <= 0, "Invalid pass size");
            passWidth = width;
            passHeight = height;
        }

    protected:
        VulkanDevice* device;
        uint32_t passWidth = 0, passHeight = 0;
    };
}
