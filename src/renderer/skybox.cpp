#include "skybox.h"

#include "shader_cache.h"
#include "vulkan/vulkan_descriptor_writer.h"

Raytracing::Skybox::Skybox(VulkanDevice* vulkanDevice, Ref<VulkanCubeMapTexture> cubemap): vulkanDevice(vulkanDevice), cubemap(cubemap)
{
    InitDescriptorSets();
}

Raytracing::Skybox::~Skybox()
{
    vkFreeDescriptorSets(vulkanDevice->GetDevice(), vulkanDevice->GetShaderDescriptorPool().GetDescriptorPool(), 1, &descriptorSet);
}

void Raytracing::Skybox::InitDescriptorSets()
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
