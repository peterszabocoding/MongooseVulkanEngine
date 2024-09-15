#pragma once

#include "math/vec3.h"

namespace Raytracing {

    class Camera {

        public:
            Camera();
            ~Camera();

            void SetResolution(unsigned int width, unsigned int height);

            const unsigned int Width() const { return viewportWidth; }
            const unsigned int Height() const { return viewportHeight; }

            const double FocalLength() const { return focalLength; }
            const double AspectRatio() const { return aspectRatio; }

            const vec3 Position() const { return position; }
            const vec3 Direction() const { return direction; }

        private:
            double focalLength = 1.0;
            double aspectRatio = 0.0;

            unsigned int viewportWidth = 1;
            unsigned int viewportHeight = 1;

            vec3 position = vec3(0.0, 0.0, 0.0);
            vec3 direction = vec3(0.0, 0.0, -1.0);

    };

}