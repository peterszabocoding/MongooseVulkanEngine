#pragma once

#include "vulkan/vulkan_descriptor_set_layout.h"
#include "vulkan/vulkan_pipeline.h"

namespace MongooseVK
{
    constexpr auto SHADER_PATH = "shader/glsl/";

    class ShaderCache {
    public:
        ShaderCache(VulkanDevice* _vulkanDevice): vulkanDevice(_vulkanDevice) { Load(); }
        ~ShaderCache() = default;

        ShaderCache(const ShaderCache& other) = delete;
        ShaderCache(ShaderCache&& other) = delete;

        void Load();

    public:
        static std::unordered_map<std::string, std::vector<uint32_t>> shaderCache;

    private:
        VulkanDevice* vulkanDevice;
    };
}
