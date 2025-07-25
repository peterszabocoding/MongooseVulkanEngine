#pragma once

#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"

#include "transform.h"
#include "math/math.h"
#include <limits>

namespace MongooseVK
{
    class Camera {
    public:
        Camera();
        explicit Camera(glm::vec3 initialPosition);
        ~Camera() = default;

        void Update();

        void SetResolution(unsigned int width, unsigned int height);
        void SetFOV(float focalLength);
        void SetTransform(const Transform& _transform);
        void SetNearPlane(float _nearPlane);
        void SetFarPlane(float _farPlane);

        glm::mat4 GetProjection() const { return projection; }
        glm::mat4 GetView() const { return view; }

        glm::vec3 GetForwardVector() const { return forwardVector; }
        glm::vec3 GetRightVector() const { return rightVector; }
        glm::vec3 GetUpVector() const { return upVector; }

        unsigned int Width() const { return viewportWidth; }
        unsigned int Height() const { return viewportHeight; }

        double GetFOV() const { return fov; }
        double GetAspectRatio() const { return aspectRatio; }

        float GetNearPlane() const { return nearPlane; }
        float GetFarPlane() const { return farPlane; }

        Transform& GetTransform() { return transform; }

    private:
        void CalculateProjection();
        void CalculateView();

    private:
        float fov = 45.0;
        float aspectRatio = 1.0;

        float nearPlane = 0.1f;
        float farPlane = 100.0f;

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
