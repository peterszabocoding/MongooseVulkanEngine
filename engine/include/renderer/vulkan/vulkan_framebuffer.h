#pragma once
#include <array>
#include <memory/resource_pool.h>

#include "vulkan_image.h"

namespace MongooseVK
{
    struct VulkanFramebuffer : PoolObject {
        VkExtent2D extent{};
        VkFramebuffer framebuffer{};
        std::array<VkImageView, 6> attachments{};
        uint32_t attachmentCount{};
        VkRenderPass renderPass{};
    };
}
