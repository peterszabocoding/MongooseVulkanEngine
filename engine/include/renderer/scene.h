#pragma once
#include <vector>

#include "Light.h"
#include "transform.h"
#include "vulkan/vulkan_material.h"
#include "vulkan/vulkan_mesh.h"
#include "vulkan/vulkan_reflection_probe.h"

namespace MongooseVK
{
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
        std::vector<VulkanMaterial> materials;

        // Environment
        Ref<VulkanReflectionProbe> reflectionProbe;

        // Lights
        DirectionalLight directionalLight;
    };
}
