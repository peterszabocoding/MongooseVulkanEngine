#pragma once

#include "renderer/scene.h"
#include "renderer/vulkan/vulkan_image.h"
#include "resource/resource.h"

namespace Raytracing {

    class VulkanDevice;
    class VulkanMesh;
    class VulkanPipeline;

    class ResourceManager {

    public:
        ResourceManager() = delete;

        static ImageResource LoadImageResource(const std::string& imagePath);
        static void ReleaseImage(const ImageResource& image);

        static void LoadPipelines(VulkanDevice* vulkanDevice);
        static Ref<VulkanPipeline> GetMainPipeline() { return mainPipeline; }

        static Ref<VulkanMesh> LoadMesh(VulkanDevice* device, const std::string& meshPath);
        static Ref<VulkanMesh> LoadMeshFromObj(VulkanDevice* device, const std::string& meshPath);
        static Ref<VulkanMesh> LoadMeshFromglTF(VulkanDevice* device, const std::string& meshPath);
        static Ref<VulkanImage> LoadTexture(VulkanDevice* device, std::string textureImagePath);

        static Scene LoadScene(VulkanDevice* device, const std::string& scenePath);

        private:
        static Ref<VulkanPipeline> mainPipeline;
    };
}
