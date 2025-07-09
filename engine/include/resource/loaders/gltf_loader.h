#pragma once

#include "renderer/scene.h"
#include "util/core.h"

namespace MongooseVK
{
    class VulkanDevice;
    class VulkanMesh;

    class GLTFLoader {
    public:
        static Ref<VulkanMesh> LoadMesh(VulkanDevice* device, const std::string& meshPath);
        static Scene LoadScene(VulkanDevice* device, const std::string& scenePath);
    };
}
