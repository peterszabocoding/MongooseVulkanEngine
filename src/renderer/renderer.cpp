#include "renderer.h"
#include "util/timer.h"

#include "math/vec3.h"
#include "platform/windows/windows_application.h"

namespace Raytracing
{
	Renderer* Renderer::Create()
	{
		return new VulkanRenderer();
	}

	void Renderer::Render(const Camera& camera, const std::vector<Hitable*>& scene)
	{
		Timer timer("Render");
		if (camera.Width() == 0 || camera.Height() == 0) return;

		OnRenderBegin(camera);

		double viewport_height = 2.0;
		double viewport_width = viewport_height * camera.AspectRatio();

		std::cout << "viewport: w: " << viewport_width << " h: " << viewport_height << std::endl;

		// Calculate the vectors across the horizontal and down the vertical viewport edges.
		vec3 viewport_u = vec3(viewport_width, 0, 0);
		vec3 viewport_v = vec3(0, viewport_height, 0);

		// Calculate the horizontal and vertical delta vectors from pixel to pixel.
		vec3 pixel_delta_u = viewport_u / camera.Width();
		vec3 pixel_delta_v = viewport_v / camera.Height();

		// Calculate the location of the upper left pixel.
		vec3 viewport_upper_left = camera.Position()
			- vec3(0, 0, camera.FocalLength()) - viewport_u / 2 - viewport_v / 2;
		vec3 pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

		// Render
		for (int y = 0; y < camera.Height(); y++)
		{
			for (int x = 0; x < camera.Width(); x++)
			{
				vec3 pixel_center = pixel00_loc + (x * pixel_delta_u) + (y * pixel_delta_v);
				vec3 ray_direction = pixel_center - camera.Position();
				Ray ray = Ray(camera.Position(), ray_direction);

				vec3 pixelColor = GetSkyColor(ray);
				for (auto hitable : scene)
				{
					HitRecord hitRecord;
					bool wasHit = hitable->Hit(ray, 0.01, 100.0, hitRecord);
					if (!wasHit) continue;

					pixelColor = RayColor(ray, hitRecord.normal);
				}

				unsigned int pixelCount = x + y * camera.Width();
				ProcessPixel(pixelCount, pixelColor);
			}
		}

		OnRenderFinished(camera);
	}

	void Renderer::Resize(int width, int height) {}

	vec3 Renderer::RayColor(const Ray& r, vec3 N)
	{
		return 0.5 * vec3(N.x() + 1, N.y() + 1, N.z() + 1);
	}

	vec3 Renderer::GetSkyColor(const Ray& r)
	{
		vec3 unit_direction = normalize(r.direction());
		auto a = 0.5 * (unit_direction.y() + 1.0);
		return (1.0 - a) * vec3(1.0, 1.0, 1.0) + a * vec3(0.5, 0.7, 1.0);
	}
}
