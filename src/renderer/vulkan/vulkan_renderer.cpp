#include "vulkan_renderer.h"
#include "resource/resource_manager.h"
#include "vulkan_mesh.h"
#include "vulkan_pipeline.h"
#include "vulkan_swapchain.h"
#include "renderer/mesh.h"
#include "util/log.h"

namespace Raytracing {

    void VulkanRenderer::Init(const int width, const int height) {
        LOG_TRACE("VulkanRenderer::Init()");
        vulkanDevice = CreateScope<VulkanDevice>(width, height, glfwWindow);

        LOG_TRACE("Build pipeline");
        graphicsPipeline = PipelineBuilder()
                .SetShaders("shader/spv/vert.spv", "shader/spv/frag.spv")
                .SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                .SetPolygonMode(VK_POLYGON_MODE_FILL)
                .EnableDepthTest()
                .DisableBlending()
                .SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                .SetMultisampling(VK_SAMPLE_COUNT_1_BIT)
                .AddPushConstant(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstantData))
                .Build(vulkanDevice.get());

        mesh = ResourceManager::LoadMesh(vulkanDevice.get(), "resources/models/viking_room.obj");
        mesh->SetMaterialIndex(0);

        cube = ResourceManager::LoadMesh(vulkanDevice.get(), "resources/models/cube.obj");
        cube->SetMaterialIndex(1);

        boomBox = ResourceManager::LoadMesh(vulkanDevice.get(), "resources/gltf/BoomBox.gltf");
        boomBox->SetMaterialIndex(2);

        transform.m_Position = glm::vec3(0.0f, 0.0f, -1.0f);
        transform.m_Rotation = glm::vec3(0.0f, 0.0f, 0.0f);

        cubeTransform.m_Position = glm::vec3(0.0f, 5.0f, -1.0f);
        cubeTransform.m_Scale = glm::vec3(1.0f, 1.0f, 1.0f);
        cubeTransform.m_Rotation = glm::vec3(0.0f, 0.0f, 0.0f);

        boomBoxTransform.m_Position = glm::vec3(2.0f, 0.5f, -1.0f);
        boomBoxTransform.m_Scale = glm::vec3(50.0f, 50.0f, 50.0f);
        boomBoxTransform.m_Rotation = glm::vec3(0.0f, 180.0f, 0.0f);

        {
            VulkanMaterial material = VulkanMaterialBuilder(vulkanDevice.get())
                .SetIndex(0)
                .SetPipeline(graphicsPipeline)
                .SetBaseColorPath("resources/textures/viking_room.png")
                .Build();

            materials.push_back(material);
        }

        {
            VulkanMaterial material = VulkanMaterialBuilder(vulkanDevice.get())
                .SetIndex(1)
                .SetPipeline(graphicsPipeline)
                .SetBaseColorPath("resources/textures/checker.png")
                .Build();

            materials.push_back(material);
        }

        {
            VulkanMaterial material = VulkanMaterialBuilder(vulkanDevice.get())
                .SetIndex(2)
                .SetPipeline(graphicsPipeline)
                .SetBaseColorPath("resources/gltf/BoomBox_baseColor.png")
                .Build();

            materials.push_back(material);
        }
    }

    void VulkanRenderer::DrawFrame(float deltaTime, Ref<Camera> camera) {
        const bool result = vulkanDevice->BeginFrame();
        if (!result) return;

        vulkanDevice->DrawMesh(camera, materials[mesh->GetMaterialIndex()], mesh, transform);
        vulkanDevice->DrawMesh(camera, materials[cube->GetMaterialIndex()], cube, cubeTransform);
        vulkanDevice->DrawMesh(camera, materials[boomBox->GetMaterialIndex()], boomBox, boomBoxTransform);
        vulkanDevice->DrawImGui();

        vulkanDevice->EndFrame();
    }

    void VulkanRenderer::IdleWait() {
        vkDeviceWaitIdle(vulkanDevice->GetDevice());
    }

    void VulkanRenderer::Resize(const int width, const int height) {
        Renderer::Resize(width, height);
        vulkanDevice->ResizeFramebuffer();
    }
}
