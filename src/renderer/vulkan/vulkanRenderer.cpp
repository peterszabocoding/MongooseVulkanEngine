#include "vulkanRenderer.h"

namespace Raytracing
{
	VulkanRenderer::~VulkanRenderer()
	{
		delete vulkanDevice;
	}

	void VulkanRenderer::Init(const int width, const int height)
	{
		vulkanDevice = new VulkanDevice();
		vulkanDevice->Init(width, height, glfwWindow);
	}

	void VulkanRenderer::DrawFrame()
	{
		//Timer timer("Render");
		vulkanDevice->Draw();
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
