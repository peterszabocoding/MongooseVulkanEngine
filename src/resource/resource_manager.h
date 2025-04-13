#pragma once

#include "resource/resource.h"

namespace Raytracing {

    class VulkanDevice;
    class VulkanMesh;

    class ResourceManager {

    public:
        ResourceManager() = delete;

        static ImageResource LoadImage(const std::string& imagePath);
        static void ReleaseImage(const ImageResource& image);

        static VulkanMesh* LoadMesh(VulkanDevice* device, const std::string& meshPath);
    };
}
