#include "vulkanRenderer.h"

#include "vulkan_pipeline.h"
#include "vulkan_texture_image.h"
#include "renderer/mesh.h"

namespace Raytracing
{
    VulkanRenderer::~VulkanRenderer()
    {
        delete vulkanDevice;
        delete graphicsPipeline;
        delete vulkanImage;
    }

    void VulkanRenderer::Init(const int width, const int height)
    {
        vulkanDevice = new VulkanDevice();
        vulkanDevice->Init(width, height, glfwWindow);

        mesh = new Mesh(vulkanDevice, Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);
        vulkanImage = new VulkanTextureImage(vulkanDevice, "textures/texture.jpg");

        graphicsPipeline = new VulkanPipeline(vulkanDevice);
        graphicsPipeline->Load("shader/spv/vert.spv", "shader/spv/frag.spv");
        graphicsPipeline->GetShader()->SetImage(vulkanImage);
    }

    void VulkanRenderer::DrawFrame()
    {
        static auto start_time = std::chrono::high_resolution_clock::now();
        const auto current_time = std::chrono::high_resolution_clock::now();
        const float time = std::chrono::duration<float>(current_time - start_time).count();

        UniformBufferObject ubo{};
        float aspectRatio = vulkanDevice->GetViewportWidth() / static_cast<float>(vulkanDevice->GetViewportHeight());

        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;

        graphicsPipeline->GetShader()->UpdateUniformBuffer(ubo);

        vulkanDevice->Draw(graphicsPipeline, mesh);
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
