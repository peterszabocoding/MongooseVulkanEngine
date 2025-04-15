#include "vulkan_renderer.h"
#include "vulkan_mesh.h"
#include "vulkan_pipeline.h"
#include "vulkan_swapchain.h"
#include "renderer/mesh.h"
#include "resource/resource_manager.h"

namespace Raytracing
{
    VulkanRenderer::~VulkanRenderer()
    {
        delete vulkanDevice;
    }

    void VulkanRenderer::Init(const int width, const int height)
    {
        std::cout << "Init Vulkan" << '\n';
        vulkanDevice = new VulkanDevice(width, height, glfwWindow);

        std::cout << "Build pipeline" << '\n';
        auto builder = PipelineBuilder();
        builder.SetShaders("shader/spv/vert.spv", "shader/spv/frag.spv");
        builder.SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        builder.SetPolygonMode(VK_POLYGON_MODE_FILL);
        builder.EnableDepthTest();
        builder.DisableBlending();
        builder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        builder.SetMultisampling(VK_SAMPLE_COUNT_1_BIT);
        builder.AddPushConstant();

        graphicsPipeline = builder.build(vulkanDevice);

        std::cout << "Load Resources" << '\n';
        mesh = ResourceManager::LoadMesh(vulkanDevice, "resources/models/viking_room.obj");
        cube = ResourceManager::LoadMesh(vulkanDevice, "resources/models/cube.obj");

        transform.m_Position = glm::vec3(0.0f, 0.0f, -1.0f);
        transform.m_Rotation = glm::vec3(0.0f, 0.0f, 0.0f);

        cubeTransform.m_Position = glm::vec3(1.0f, 1.5f, -2.0f);
        cubeTransform.m_Scale = glm::vec3(0.25f, 0.25f, 0.25f);

        texture = ResourceManager::LoadTexture(vulkanDevice, "resources/textures/viking_room.png");
        checkerTexture = ResourceManager::LoadTexture(vulkanDevice, "resources/textures/checker.png");

        material = VulkanMaterialBuilder(vulkanDevice)
                .SetIndex(0)
                .SetShader(graphicsPipeline->GetShader())
                .SetBaseColorTexture(texture)
                .SetParams({{1.0f, 0.0f, 0.0f}})
                .Build();

        checkerMaterial = VulkanMaterialBuilder(vulkanDevice)
                .SetIndex(1)
                .SetShader(graphicsPipeline->GetShader())
                .SetBaseColorTexture(checkerTexture)
                .SetParams({{0.0f, 1.0f, 0.0f}})
                .Build();
    }

    void VulkanRenderer::DrawFrame(float deltaTime, Ref<Camera> camera)
    {
        const bool result = vulkanDevice->BeginFrame();
        if (!result) return;

        vulkanDevice->DrawMesh(graphicsPipeline, camera, mesh, transform, material);
        vulkanDevice->DrawMesh(graphicsPipeline, camera, cube, cubeTransform, checkerMaterial);
        vulkanDevice->DrawImGui();

        vulkanDevice->EndFrame();
    }

    void VulkanRenderer::IdleWait()
    {
        vkDeviceWaitIdle(vulkanDevice->GetDevice());
    }

    void VulkanRenderer::Resize(const int width, const int height)
    {
        Renderer::Resize(width, height);
        vulkanDevice->ResizeFramebuffer();
    }
}
