#pragma once
#include "util/core.h"
#include "vulkan/vulkan_descriptor_set_layout.h"
#include "vulkan/vulkan_pipeline.h"

namespace MongooseVK
{
    constexpr auto SHADER_PATH = "shader/glsl/";

    struct DescriptorSetLayouts {
        DescriptorSetLayoutHandle cubemapDescriptorSetLayout;
        DescriptorSetLayoutHandle presentDescriptorSetLayout;
        DescriptorSetLayoutHandle irradianceDescriptorSetLayout;
        DescriptorSetLayoutHandle ssaoDescriptorSetLayout;
    };

    struct DescriptorSets {
        VkDescriptorSet cubemapDescriptorSet;
        VkDescriptorSet irradianceDescriptorSet;
        VkDescriptorSet presentDescriptorSet;
    };

    class ShaderCache {
    public:
        ShaderCache(VulkanDevice* _vulkanDevice): vulkanDevice(_vulkanDevice) { Load(); }
        ~ShaderCache() = default;

        ShaderCache(const ShaderCache& other) = delete;
        ShaderCache(ShaderCache&& other) = delete;

        void Load();

    public:
        static DescriptorSetLayouts descriptorSetLayouts;
        static DescriptorSets descriptorSets;
        static std::unordered_map<std::string, std::vector<uint32_t>> shaderCache;

    private:
        void LoadShaders();
        void LoadDescriptorLayouts();
        void LoadPipelines();
        void LoadRenderpasses();

    private:
        VulkanDevice* vulkanDevice;
    };
}
