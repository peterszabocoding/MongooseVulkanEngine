#include "renderer/camera.h"
#include "util/log.h"

namespace MongooseVK
{
    Camera::Camera()
    {
        CalculateProjection();
        Update();
    }

    Camera::Camera(const glm::vec3 initialPosition)
    {
        CalculateProjection();
        Update();
        transform.m_Position = initialPosition;
    }

    void Camera::Update()
    {
        forwardVector = transform.GetForwardDirection();
        rightVector = normalize(cross(forwardVector, glm::vec3(0.0f, 1.0f, 0.0f)));
        upVector = normalize(cross(rightVector, forwardVector));

        CalculateView();
    }

    void Camera::SetResolution(const unsigned int width, const unsigned int height)
    {
        viewportWidth = width;
        viewportHeight = height;
        aspectRatio = static_cast<float>(viewportWidth) / viewportHeight;
        CalculateProjection();
    }

    void Camera::SetFOV(const float focalLength)
    {
        if (isEqual(fov, focalLength)) return;

        fov = focalLength;
        CalculateProjection();
    }

    void Camera::SetTransform(const Transform& _transform)
    {
        transform = _transform;
    }

    void Camera::SetNearPlane(const float _nearPlane)
    {
        if (isEqual(nearPlane, _nearPlane)) return;

        nearPlane = _nearPlane;
        CalculateProjection();
    }

    void Camera::SetFarPlane(const float _farPlane)
    {
        if (isEqual(farPlane, _farPlane)) return;

        farPlane = _farPlane;
        CalculateProjection();
    }

    void Camera::CalculateProjection()
    {
        projection = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
        projection[1][1] *= -1;
    }

    void Camera::CalculateView()
    {
        view = lookAt(transform.m_Position, transform.m_Position + forwardVector, upVector);
    }
}
