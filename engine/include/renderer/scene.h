#pragma once
#include <vector>
#include <resource/resource.h>
#include <util/core.h>

#include "Light.h"
#include "transform.h"

namespace MongooseVK
{
    class VulkanMaterial;
    class VulkanMesh;

    struct Renderable {
        Ref<VulkanMesh> mesh;
        Ref<VulkanMaterial> material;
        Transform transform;
    };

    struct Scene {
        std::string name = "";
        std::string filePath = "";

        std::string scenePath = "";
        std::string skyboxPath = "";

        // Render objects
        std::vector<Transform> transforms;
        std::vector<Ref<VulkanMesh>> meshes;
        std::vector<MaterialHandle> materials;

        // Lights
        DirectionalLight directionalLight;
    };
}
