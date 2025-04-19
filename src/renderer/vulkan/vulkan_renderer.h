#pragma once

#include "renderer/renderer.h"
#include "vulkan_device.h"
#include "vulkan_material.h"
#include "renderer/components.h"

namespace Raytracing
{
	class VulkanRenderer : public Renderer
	{
	public:
		VulkanRenderer() = default;
		~VulkanRenderer() override = default;

		virtual void Init(int width, int height) override;

		virtual void ProcessPixel(unsigned int pixelCount, vec3 pixelColor) override {}
		virtual void OnRenderBegin(const Camera& camera) override {}
		virtual void OnRenderFinished(const Camera& camera) override {}

		virtual void IdleWait() override;
		virtual void Resize(int width, int height) override;
		virtual void DrawFrame(float deltaTime, Ref<Camera> camera) override;

		VulkanDevice* GetVulkanDevice() const { return vulkanDevice.get(); }

	private:
		Scope<VulkanDevice> vulkanDevice;
		Ref<VulkanPipeline> graphicsPipeline;

		Ref<VulkanMesh> mesh;
		Ref<VulkanMesh> cube;
		Ref<VulkanMesh> boomBox;

		Transform transform;
		Transform cubeTransform;
		Transform boomBoxTransform;

		std::vector<VulkanMaterial> materials;

		Ref<VulkanImage> texture;
		Ref<VulkanImage> checkerTexture;
	};
}
