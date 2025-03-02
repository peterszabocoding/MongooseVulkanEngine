#pragma once

#include <vector>

#include "math/vec3.h"
#include "math/ray.h"

#include "camera.h"
#include "math/hitable.h"
#include "GLFW/glfw3.h"

namespace Raytracing
{
	class Renderer
	{
	public:
		virtual ~Renderer() = default;
		virtual void Render(const Camera& camera, const std::vector<Hitable*>& scene);

		virtual void Init(int width, int height) = 0;
		virtual void SetupImGui(const int width, const int height) = 0;
		virtual void Resize(int width, int height);

		virtual void IdleWait() {}
		virtual void DrawFrame() {}
		virtual void DrawUi() {}

		virtual void SetGLFWwindow(GLFWwindow* window) { glfwWindow = window; }

	protected:
		virtual void ProcessPixel(unsigned int pixelCount, vec3 pixelColor) = 0;
		virtual void OnRenderBegin(const Camera& camera) = 0;
		virtual void OnRenderFinished(const Camera& camera) = 0;

	private:
		vec3 GetSkyColor(const Ray& r);
		vec3 RayColor(const Ray& r, vec3 N);

	protected:
		GLFWwindow* glfwWindow;
	};
}
