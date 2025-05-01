#include "vulkan_renderer.h"
#include "resource/resource_manager.h"
#include "vulkan_pipeline.h"
#include "vulkan_swapchain.h"
#include "renderer/vulkan/vulkan_device.h"
#include "util/log.h"

namespace Raytracing
{
    void VulkanRenderer::Init(const int width, const int height)
    {
        LOG_TRACE("VulkanRenderer::Init()");
        vulkanDevice = CreateScope<VulkanDevice>(width, height, glfwWindow);

        LOG_TRACE("Build pipelines");
        ResourceManager::LoadPipelines(vulkanDevice.get(), vulkanDevice->GetRenderPass());

        LOG_TRACE("Load skybox");
        //auto cubeMapTexture = ResourceManager::LoadHDRCubeMap(vulkanDevice.get(), "resources/environment/aristea_wreck_puresky_4k.hdr");

        LOG_TRACE("Load scene");
        completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/sponza/Sponza.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/gltf/multiple_spheres.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/chess/ABeautifulGame.gltf");

        transform.m_Position = glm::vec3(0.0f, 0.0f, -1.0f);
    }

    void VulkanRenderer::DrawFrame(float deltaTime, Ref<Camera> camera)
    {
        const bool result = vulkanDevice->BeginFrame();
        if (!result) return;

        for (size_t i = 0; i < completeScene.meshes.size(); i++)
        {
            vulkanDevice->DrawMesh(camera, completeScene.transforms[i], completeScene.meshes[i]);
        }

        //vulkanDevice->DrawImGui();
        vulkanDevice->EndFrame();
    }

    void VulkanRenderer::IdleWait()
    {
        vkDeviceWaitIdle(vulkanDevice->GetDevice());
    }

    void VulkanRenderer::Resize(const int width, const int height)
    {
        Renderer::Resize(width, height);
        vulkanDevice->ResizeFramebuffer(width, height);
    }
}
