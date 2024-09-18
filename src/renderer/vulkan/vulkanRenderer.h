#pragma once

#include "renderer/renderer.h"
#include "vulkan/vulkan.h"

namespace Raytracing
{
	class VulkanRenderer : public Renderer
	{
	public:
		VulkanRenderer() = default;
		virtual ~VulkanRenderer() override = default;

		virtual void ProcessPixel(unsigned int pixelCount, vec3 pixelColor) override;
		virtual void OnRenderBegin(const Camera& camera) override;
		virtual void OnRenderFinished(const Camera& camera) override;

	private:

	private:
	};
}
