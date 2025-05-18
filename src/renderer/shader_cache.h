#pragma once
#include "util/core.h"
#include "vulkan/vulkan_buffer.h"
#include "vulkan/vulkan_descriptor_set_layout.h"
#include "vulkan/vulkan_pipeline.h"

namespace Raytracing
{
    struct DescriptorSetLayouts {
        Ref<VulkanDescriptorSetLayout> cubemapDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> materialDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> transformDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> lightsDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> presentDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> irradianceDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> reflectionDescriptorSetLayout;
    };

    struct DescriptorSets {
        VkDescriptorSet cubemapDescriptorSet;
        VkDescriptorSet transformDescriptorSet;
        VkDescriptorSet irradianceDescriptorSet;
        VkDescriptorSet reflectionDescriptorSet;
        std::vector<VkDescriptorSet> presentDescriptorSets;
        std::vector<VkDescriptorSet> lightsDescriptorSets;
    };

    struct Pipelines {
        Ref<VulkanPipeline> ibl_brdf;
        Ref<VulkanPipeline> ibl_irradianceMap;
        Ref<VulkanPipeline> ibl_prefilter;
    };

    struct Renderpass {
        Ref<VulkanRenderPass> iblPreparePass{};
    };

    class ShaderCache {
    public:
        ShaderCache(VulkanDevice* _vulkanDevice): vulkanDevice(_vulkanDevice) { Load(); }
        ~ShaderCache() = default;

        ShaderCache(const ShaderCache& other) = delete;
        ShaderCache(ShaderCache&& other) = delete;

        void Load();

    public:
        static Pipelines pipelines;
        static DescriptorSetLayouts descriptorSetLayouts;
        static DescriptorSets descriptorSets;
        static Renderpass renderpasses;

    private:
        void LoadDescriptorLayouts();
        void LoadPipelines();
        void LoadRenderpasses();

    private:
        VulkanDevice* vulkanDevice;
    };
}
