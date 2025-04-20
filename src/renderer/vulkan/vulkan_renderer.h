#pragma once

#include "renderer/renderer.h"
#include "vulkan_device.h"
#include "vulkan_material.h"
#include "renderer/scene.h"
#include "renderer/transform.h"

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

		Ref<VulkanMesh> scene;

		Scene completeScene;
		Transform transform;
	};
}
