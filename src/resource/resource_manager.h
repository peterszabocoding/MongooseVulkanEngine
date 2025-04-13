#pragma once

#include "renderer/vulkan/vulkan_image.h"
#include "resource/resource.h"

namespace Raytracing {

    class VulkanDevice;
    class VulkanMesh;

    class ResourceManager {

    public:
        ResourceManager() = delete;

        static ImageResource LoadImage(const std::string& imagePath);
        static void ReleaseImage(const ImageResource& image);

        static Ref<VulkanMesh> LoadMesh(VulkanDevice* device, const std::string& meshPath);
        static Ref<VulkanImage> LoadTexture(VulkanDevice* device, std::string textureImagePath);
    };
}
