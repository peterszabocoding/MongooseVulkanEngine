#pragma once

#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"

#include "transform.h"
#include "math/math.h"
#include <limits>

namespace Raytracing
{
    class Camera {
    public:
        Camera()
        {
            CalculateProjection();
            Update();
        };
        ~Camera() = default;

        void Update()
        {
            forwardVector = transform.GetForwardDirection();
            rightVector = normalize(cross(forwardVector, glm::vec3(0.0f, 1.0f, 0.0f)));
            upVector = normalize(cross(rightVector, forwardVector));

            CalculateView();
        }

        void SetResolution(const unsigned int width, const unsigned int height)
        {
            viewportWidth = width;
            viewportHeight = height;
            aspectRatio = static_cast<float>(viewportWidth) / viewportHeight;
            CalculateProjection();
        }

        void SetFOV(const float focalLength)
        {
            if (isEqual(fov, focalLength)) return;

            fov = focalLength;
            CalculateProjection();
        }

        void SetTransform(const Transform& _transform)
        {
            transform = _transform;
        }

        void SetNearPlane(const float _nearPlane)
        {
            nearPlane = _nearPlane;
            CalculateView();
        }

        void SetFarPlane(const float _farPlane)
        {
            farPlane = _farPlane;
            CalculateView();
        }

        glm::mat4 GetProjection() const { return projection; }
        glm::mat4 GetView() const { return view; }

        glm::vec3 GetForwardVector() const { return forwardVector; }
        glm::vec3 GetRightVector() const { return rightVector; }
        glm::vec3 GetUpVector() const { return upVector; }

        unsigned int Width() const { return viewportWidth; }
        unsigned int Height() const { return viewportHeight; }

        double GetFOV() const { return fov; }
        double GetAspectRatio() const { return aspectRatio; }

        Transform& GetTransform() { return transform; }

    private:
        void CalculateProjection()
        {
            projection = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
            projection[1][1] *= -1;
        }

        void CalculateView()
        {
            view = lookAt(transform.m_Position, transform.m_Position + forwardVector, upVector);
        }

    private:
        float fov = 45.0;
        float aspectRatio = 1.0;

        float nearPlane = 0.1f;
        float farPlane = 10000.0f;

        unsigned int viewportWidth = 1;
        unsigned int viewportHeight = 1;

        Transform transform;

        glm::mat4 projection = glm::mat4(1.0f);
        glm::mat4 view = glm::mat4(1.0f);

        glm::vec3 forwardVector;
        glm::vec3 upVector;
        glm::vec3 rightVector;
    };
}
