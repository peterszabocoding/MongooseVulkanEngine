#pragma once

#include "renderer/bitmap.h"
#include "renderer/scene.h"
#include "resource/resource.h"

namespace MongooseVK
{
    class VulkanCubeMapTexture;
    class VulkanTexture;
    class VulkanDevice;
    class VulkanMesh;
    class VulkanPipeline;

    class ResourceManager {
    public:
        ResourceManager() = delete;

        static ImageResource LoadImageResource(const std::string& imagePath);
        static ImageResource LoadHDRResource(const std::string& hdrPath);

        static void ReleaseImage(const ImageResource& image);

        static Ref<VulkanMesh> LoadMesh(VulkanDevice* device, const std::string& meshPath);

        static void LoadTexture(VulkanDevice* device, const std::string& textureImagePath, std::vector<TextureHandle>* textureHandles);

        static Bitmap LoadHDRCubeMapBitmap(VulkanDevice* device, const std::string& hdrPath);
        static void LoadAndSaveHDR(const std::string& hdrPath);

        static SceneGraph* LoadSceneGraph(VulkanDevice* device, const std::string& scenePath, const std::string& skyboxPath);
    };
}
