#pragma once

#include "renderer/renderer.h"
#include "vulkan_device.h"
#include "vulkan_image.h"

namespace Raytracing
{
	class VulkanRenderer : public Renderer
	{
	public:
		VulkanRenderer() = default;
		~VulkanRenderer() override;

		virtual void Init(int width, int height) override;

		virtual void ProcessPixel(unsigned int pixelCount, vec3 pixelColor) override {}
		virtual void OnRenderBegin(const Camera& camera) override {}
		virtual void OnRenderFinished(const Camera& camera) override {}

		virtual void IdleWait() override;
		virtual void Resize(int width, int height) override;
		virtual void DrawFrame() override;

		VulkanDevice* GetVulkanDevice() const { return vulkanDevice; }

	private:
		VulkanDevice* vulkanDevice;
		VulkanPipeline* graphicsPipeline;
		VulkanImage* vulkanImage;
		VulkanMesh* mesh;
	};
}
