#pragma once
#include "vulkan_cube_map_texture.h"

namespace MongooseVK
{
    class VulkanReflectionProbe {
    public:
        VulkanReflectionProbe(VulkanDevice* device, TextureHandle prefilterMap);
        ~VulkanReflectionProbe();

    public:
        TextureHandle prefilterMap;

    private:
        VulkanDevice* device;
    };
}
