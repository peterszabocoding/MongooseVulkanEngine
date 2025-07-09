#pragma once
#include "vulkan_cube_map_texture.h"

namespace Raytracing
{
    class VulkanReflectionProbe {
    public:
        VulkanReflectionProbe(VulkanDevice* device, const Ref<VulkanCubeMapTexture>& prefilterMap, const Ref<VulkanTexture>& brdfLUT);
        ~VulkanReflectionProbe();

    public:
        Ref<VulkanCubeMapTexture> prefilterMap;
        Ref<VulkanTexture> brdfLUT;
        VkDescriptorSet descriptorSet{};

    private:
        VulkanDevice* device;
    };
}
