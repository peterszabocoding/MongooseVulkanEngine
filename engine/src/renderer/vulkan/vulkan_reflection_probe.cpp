#include "renderer/vulkan/vulkan_reflection_probe.h"

#include <renderer/vulkan/vulkan_texture.h>

#include "renderer/vulkan/vulkan_descriptor_writer.h"
#include "renderer/shader_cache.h"

namespace MongooseVK
{
    VulkanReflectionProbe::VulkanReflectionProbe(VulkanDevice* _device,
                                                 TextureHandle _prefilterMap): device(_device),
                                                                          prefilterMap(_prefilterMap)
    {
        VulkanTexture* prefilterMapTexture = device->GetTexture(prefilterMap);
        VkDescriptorImageInfo prefilterMapInfo{};
        prefilterMapInfo.sampler = prefilterMapTexture->GetSampler();
        prefilterMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        prefilterMapInfo.imageView = prefilterMapTexture->GetImageView();

        VulkanDescriptorWriter(*ShaderCache::descriptorSetLayouts.reflectionDescriptorSetLayout, device->GetShaderDescriptorPool())
                .WriteImage(0, &prefilterMapInfo)
                .Build(ShaderCache::descriptorSets.reflectionDescriptorSet);
    }

    VulkanReflectionProbe::~VulkanReflectionProbe() {}
}
