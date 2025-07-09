#pragma once
#include "util/core.h"

namespace Raytracing {
    class VulkanDevice;
    class VulkanMesh;

    class ObjLoader {
        public:
          static Ref<VulkanMesh> LoadMesh(VulkanDevice* device, const std::string& meshPath);
    };

}
