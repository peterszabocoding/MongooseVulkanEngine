#pragma once

#include "renderer/renderer.h"
#include "vulkan_device.h"
#include "vulkan_image.h"
#include "renderer/components.h"

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
		virtual void DrawFrame(float deltaTime, Ref<Camera> camera) override;

		VulkanDevice* GetVulkanDevice() const { return vulkanDevice; }

	private:
		VulkanDevice* vulkanDevice;
		Ref<VulkanPipeline> graphicsPipeline;
		Ref<VulkanImage> vulkanImage;

		VulkanMesh* mesh;
		Transform transform;
	};
}
