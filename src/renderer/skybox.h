#pragma once
#include "vulkan/vulkan_cube_map_texture.h"

namespace Raytracing
{
    class Skybox {
    public:
        Skybox(VulkanDevice* vulkanDevice, Ref<VulkanCubeMapTexture> cubemap);
        ~Skybox();

        Ref<VulkanCubeMapTexture> GetCubemap() { return cubemap; }

    private:
        void InitDescriptorSets();

    public:
        VkDescriptorSet descriptorSet;

    private:
        VulkanDevice* vulkanDevice;
        Ref<VulkanCubeMapTexture> cubemap;
    };
}
