#pragma once

#include <vector>

#include "math/vec3.h"
#include "math/ray.h"

#include "camera.h"
#include "math/hitable.h"

namespace Raytracing {

    struct Vertex {
        vec3 position;
        vec3 texCoord;
        vec3 color = vec3();
    };

    class Renderer {

        public:
            virtual ~Renderer() {};
            virtual void Render(const Camera& camera, const std::vector<Hitable*>& scene);

        protected:
            virtual void ProcessPixel(unsigned int pixelCount, vec3 pixelColor) = 0;
            virtual void OnRenderBegin(const Camera& camera) = 0;
            virtual void OnRenderFinished(const Camera& camera) = 0;

        private:
            vec3 GetSkyColor(const Ray& r);
            vec3 RayColor(const Ray& r, vec3 N);

    };
}