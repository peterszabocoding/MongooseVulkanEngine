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
        Scene LoadScene(VulkanDevice* device, const std::string& scenePath);

        std::vector<TextureHandle> LoadTextures(tinygltf::Model& model,
                                                VulkanDevice* device,
                                                const std::filesystem::path& parentPath);

        std::vector<MaterialHandle> LoadMaterials(const tinygltf::Model& model,
                                                  VulkanDevice* device,
                                                  const std::vector<TextureHandle>& textureHandles);

    private:
        std::vector<std::future<void>> textureFutures;
    };
}
