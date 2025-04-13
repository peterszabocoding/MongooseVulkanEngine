#include "vulkan_renderer.h"
#include "vulkan_mesh.h"
#include "vulkan_pipeline.h"
#include "vulkan_swapchain.h"
#include "renderer/mesh.h"
#include "resource/resource_manager.h"

namespace Raytracing {
    VulkanRenderer::~VulkanRenderer() {
        delete vulkanDevice;
    }

    void VulkanRenderer::Init(const int width, const int height) {
        vulkanDevice = new VulkanDevice(width, height, glfwWindow);

        mesh = new VulkanMesh(vulkanDevice, Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);
        mesh2 = new VulkanMesh(vulkanDevice, Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);

        transform.m_Position = glm::vec3(0.0f, 0.0f, -1.0f);
        transform.m_Rotation = glm::vec3(0.0f, 0.0f, 0.0f);

        transform2.m_Position = glm::vec3(0.0f, 1.0f, 0.0f);
        transform2.m_Rotation = glm::vec3(0.0f, 0.0f, 0.0f);
        transform2.m_Scale = glm::vec3(0.5f, 0.5f, 0.5f);

        const ImageResource imageResource = ResourceManager::LoadImage("textures/texture.jpg");

        VulkanTextureImageBuilder textureImageBuilder;
        textureImageBuilder.SetData(imageResource.data, imageResource.size);
        textureImageBuilder.SetResolution(imageResource.width, imageResource.height);
        textureImageBuilder.SetFormat(VK_FORMAT_R8G8B8A8_SRGB);
        textureImageBuilder.SetFilter(VK_FILTER_LINEAR, VK_FILTER_LINEAR);
        textureImageBuilder.SetTiling(VK_IMAGE_TILING_OPTIMAL);

        vulkanImage = textureImageBuilder.Build(vulkanDevice);

        ResourceManager::ReleaseImage(imageResource);

        auto builder = PipelineBuilder();
        builder.SetShaders("shader/spv/vert.spv", "shader/spv/frag.spv");
        builder.SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
        builder.SetPolygonMode(VK_POLYGON_MODE_FILL);
        builder.EnableDepthTest();
        builder.DisableBlending();
        builder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        builder.SetMultisampling(VK_SAMPLE_COUNT_1_BIT);
        builder.AddPushConstant();

        graphicsPipeline = builder.build(vulkanDevice);
        graphicsPipeline->GetShader()->SetImage(vulkanImage);
    }

    void VulkanRenderer::DrawFrame(float deltaTime, Ref<Camera> camera) {
        const bool result = vulkanDevice->BeginFrame();
        if (!result) return;

        //transform.m_Rotation += deltaTime * 90.0f * glm::vec3(0.0f, 1.0f, 0.0f);
        transform2.m_Rotation += deltaTime * 90.0f * glm::vec3(0.0f, 1.0f, 0.0f);

        vulkanDevice->DrawMesh(graphicsPipeline, camera, mesh, transform);
        vulkanDevice->DrawMesh(graphicsPipeline, camera, mesh2, transform2);

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
