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

        LOG_TRACE("Build pipelines");
        ResourceManager::LoadPipelines(vulkanDevice.get());

        LOG_TRACE("Load meshes");
        //scene = ResourceManager::LoadMesh(vulkanDevice.get(), "resources/sponza/Sponza.gltf");
        scene = ResourceManager::LoadMesh(vulkanDevice.get(), "resources/chess/ABeautifulGame.gltf");

        transform.m_Position = glm::vec3(0.0f, 0.0f, -1.0f);
    }

    void VulkanRenderer::DrawFrame(float deltaTime, Ref<Camera> camera) {
        const bool result = vulkanDevice->BeginFrame();
        if (!result) return;

        vulkanDevice->DrawMesh(camera, transform, scene);
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
