#pragma once
#include <iostream>
#include <vector>
#include <memory/resource_pool.h>
#include <resource/resource.h>
#include <util/core.h>

#include "Light.h"
#include "transform.h"

#include "vulkan/vulkan_mesh.h"

namespace MongooseVK
{
    struct Renderable {
        Ref<VulkanMesh> mesh;
        Ref<VulkanMaterial> material;
        Transform transform;
    };

    typedef int64_t SceneNodeHandle;
    static SceneNodeHandle INVALID_SCENE_NODE_HANDLE = -1;

    struct SceneNode {
        SceneNodeHandle handle = INVALID_SCENE_NODE_HANDLE;
        SceneNodeHandle parent = INVALID_SCENE_NODE_HANDLE;
        SceneNodeHandle firstChild = INVALID_SCENE_NODE_HANDLE;
        SceneNodeHandle nextSibling = INVALID_SCENE_NODE_HANDLE;
        SceneNodeHandle lastSibling = INVALID_SCENE_NODE_HANDLE;
        uint32_t level = 0;
    };

    struct SceneGraph {
        std::vector<SceneNode> nodes{};
        std::vector<VulkanMesh*> meshes{};
        std::vector<Transform> transforms{};

        std::vector<std::string> names{};

        std::vector<MaterialHandle> materials;

        TextureHandle skyboxTexture = INVALID_TEXTURE_HANDLE;

        DirectionalLight directionalLight{};

        SceneGraph()
        {
            SceneNode rootNode = {};
            rootNode.handle = 0;
            rootNode.level = 0;
            rootNode.parent = INVALID_SCENE_NODE_HANDLE;
            nodes.push_back(rootNode);
            transforms.push_back(Transform());
            meshes.push_back(nullptr);
            names.push_back("Root");
        }

        ~SceneGraph()
        {
            for (const auto& mesh : meshes) delete mesh;
        }

        Transform GetTransform(SceneNodeHandle handle)
        {
            SceneNode node = nodes[handle];
            Transform transform = transforms[handle];

            while (node.parent != INVALID_SCENE_NODE_HANDLE)
            {
                Transform parentTransform = transforms[node.parent];
                transform.m_Position += parentTransform.m_Position;
                transform.m_Rotation += parentTransform.m_Rotation;
                transform.m_Scale *= parentTransform.m_Scale;

                node = nodes[node.parent];
            }

            return transform;
        }

        void Print()
        {
            PrintNode(nodes[0]);
        }

        void PrintNode(const SceneNode& node)
        {
            std::cout << "Handle: " << node.handle << " Name: " << names[node.handle] << std::endl;

            if (node.firstChild != INVALID_SCENE_NODE_HANDLE)
            {
                std::cout << "########### CHILDREN #############" << std::endl;
                PrintNode(nodes[node.firstChild]);
                std::cout << "##################################" << std::endl;
            }

            if (node.nextSibling != INVALID_SCENE_NODE_HANDLE)
                PrintNode(nodes[node.nextSibling]);
        }
    };
}
