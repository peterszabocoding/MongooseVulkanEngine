#pragma once

#include <filesystem>
#include <future>
#include <renderer/vulkan/vulkan_texture.h>

#include <tiny_gltf/tiny_gltf.h>

#include "renderer/scene.h"
#include "util/core.h"

namespace MongooseVK
{
    class VulkanDevice;
    class VulkanMesh;

    class GLTFLoader {
    public:
        GLTFLoader() = default;
        ~GLTFLoader() = default;

        Ref<VulkanMesh> LoadMesh(VulkanDevice* device, const std::string& meshPath);

        SceneGraph* LoadSceneGraph(VulkanDevice* device, const std::string& scenePath);

    private:
        std::vector<TextureHandle> LoadTextures(VulkanDevice* device,
                                                const tinygltf::Model& model,
                                                const std::filesystem::path& parentPath);

        std::vector<MaterialHandle> LoadMaterials(VulkanDevice* device,
                                                  const tinygltf::Model& model, const std::filesystem::path& parentPath);

    };
}
