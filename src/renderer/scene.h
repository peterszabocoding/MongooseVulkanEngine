#pragma once
#include <vector>

#include "vulkan/vulkan_skybox.h"
#include "transform.h"
#include "vulkan/vulkan_material.h"
#include "vulkan/vulkan_mesh.h"
#include "vulkan/vulkan_reflection_probe.h"

namespace Raytracing
{
    struct Renderable {
        Ref<VulkanMesh> mesh;
        Ref<VulkanMaterial> material;
        Transform transform;
    };

    struct Scene {
        std::string name = "";
        std::string filePath = "";

        std::vector<Renderable> renderables;

        std::vector<VulkanMaterial> materials;
        std::vector<Ref<VulkanMesh>> meshes;
        std::vector<Transform> transforms;

        Scope<VulkanSkybox> skybox;

        Ref<VulkanReflectionProbe> reflectionProbe;
    };
}
