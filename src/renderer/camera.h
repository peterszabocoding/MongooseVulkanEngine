#pragma once

#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"

#include "transform.h"

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
            rightVector = glm::normalize(glm::cross(forwardVector, glm::vec3(0.0f, 1.0f, 0.0f)));
            upVector = glm::normalize(glm::cross(rightVector, forwardVector));

            CalculateView();
        }

        void SetResolution(const unsigned int width, const unsigned int height)
        {
            viewportWidth = width;
            viewportHeight = height;
            aspectRatio = static_cast<float>(viewportWidth) / viewportHeight;
            CalculateProjection();
        }

        void SetFocalLength(const float focalLength)
        {
            this->focalLength = focalLength;
            CalculateView();
        }

        void SetTransform(const Transform& transform)
        {
            this->transform = transform;
        }

        void SetNearPlane(const float nearPlane)
        {
            this->nearPlane = nearPlane;
            CalculateView();
        }

        void SetFarPlane(const float farPlane)
        {
            this->farPlane = farPlane;
            CalculateView();
        }

        glm::mat4 GetProjection() const { return projection; }
        glm::mat4 GetView() const { return view; }

        glm::vec3 GetForwardVector() const { return forwardVector; }
        glm::vec3 GetRightVector() const { return rightVector; }
        glm::vec3 GetUpVector() const { return upVector; }

        unsigned int Width() const { return viewportWidth; }
        unsigned int Height() const { return viewportHeight; }

        double FocalLength() const { return focalLength; }
        double AspectRatio() const { return aspectRatio; }

        Transform& GetTransform() { return transform; }

    private:
        void CalculateProjection()
        {
            projection = glm::perspective(glm::radians(focalLength), aspectRatio, nearPlane, farPlane);
            projection[1][1] *= -1;
        }

        void CalculateView()
        {
            view = glm::lookAt(transform.m_Position, transform.m_Position + forwardVector, upVector);
        }

    private:
        float focalLength = 45.0;
        float aspectRatio = 1.0;

        unsigned int viewportWidth = 1;
        unsigned int viewportHeight = 1;

        float nearPlane = 0.1f;
        float farPlane = 10.0f;

        Transform transform;

        glm::mat4 projection = glm::mat4(1.0f);
        glm::mat4 view = glm::mat4(1.0f);

        glm::vec3 forwardVector;
        glm::vec3 upVector;
        glm::vec3 rightVector;
    };
}
