#pragma once
#include "vulkan_cube_map_texture.h"

namespace MongooseVK
{
    class VulkanSkybox {
    public:
        VulkanSkybox(VulkanDevice* vulkanDevice, const Ref<VulkanCubeMapTexture>& cubemap);
        ~VulkanSkybox();

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
