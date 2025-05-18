#include "vulkan_reflection_probe.h"

#include "vulkan_descriptor_writer.h"
#include "renderer/shader_cache.h"

namespace Raytracing
{
    VulkanReflectionProbe::VulkanReflectionProbe(VulkanDevice* _device,
                                                 const Ref<VulkanCubeMapTexture>& _prefilterMap,
                                                 const Ref<VulkanTexture>& _brdfLUT): device(_device), prefilterMap(_prefilterMap),
                                                                                      brdfLUT(_brdfLUT)
    {
        VkDescriptorImageInfo prefilterMapInfo{};
        prefilterMapInfo.sampler = prefilterMap->GetSampler();
        prefilterMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        prefilterMapInfo.imageView = prefilterMap->GetImageView();

        VkDescriptorImageInfo brdfLUTInfo{};
        brdfLUTInfo.sampler = brdfLUT->GetSampler();
        brdfLUTInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        brdfLUTInfo.imageView = brdfLUT->GetImageView();

        VulkanDescriptorWriter(*ShaderCache::descriptorSetLayouts.reflectionDescriptorSetLayout, device->GetShaderDescriptorPool())
                .WriteImage(0, &prefilterMapInfo)
                .WriteImage(1, &brdfLUTInfo)
                .Build(descriptorSet);
    }

    VulkanReflectionProbe::~VulkanReflectionProbe()
    {
        vkFreeDescriptorSets(device->GetDevice(), device->GetShaderDescriptorPool().GetDescriptorPool(), 1, &descriptorSet);
    }
}
