#include "vulkan_skybox.h"

#include "../shader_cache.h"
#include "vulkan_descriptor_writer.h"

Raytracing::VulkanSkybox::VulkanSkybox(VulkanDevice* vulkanDevice, const Ref<VulkanCubeMapTexture>& _cubemap): vulkanDevice(vulkanDevice),
    cubemap(_cubemap)
{
    InitDescriptorSets();
}

Raytracing::VulkanSkybox::~VulkanSkybox()
{
    vkFreeDescriptorSets(vulkanDevice->GetDevice(), vulkanDevice->GetShaderDescriptorPool().GetDescriptorPool(), 1, &descriptorSet);
}

void Raytracing::VulkanSkybox::InitDescriptorSets()
{
    VkDescriptorImageInfo info{
        cubemap->GetSampler(),
        cubemap->GetImageView(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VulkanDescriptorWriter(*ShaderCache::descriptorSetLayouts.cubemapDescriptorSetLayout, vulkanDevice->GetShaderDescriptorPool())
            .WriteImage(0, &info)
            .Build(descriptorSet);
}
