#include "camera.h"

namespace Raytracing {

    Camera::Camera()
    {
    }

    Camera::~Camera()
    {
    }

    void Camera::SetResolution(unsigned int width, unsigned int height)
    {
        viewportWidth = width;
        viewportHeight = height;

        aspectRatio = (float) viewportWidth / viewportHeight;
    }
}
