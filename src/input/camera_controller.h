#pragma once
#include "renderer/camera.h"
#include "util/core.h"

namespace Raytracing
{
    class CameraController {
    public:
        CameraController() = default;
        ~CameraController() = default;

        void Update(float deltaTime);
        void SetCamera(Ref<Camera> camera) { this->camera = camera; }

    private:
        Ref<Camera> camera;

        bool m_IsCameraMoving = false;
        glm::vec2 mouseDelta = {0.0f, 0.0f};
        glm::vec2 lastMousePos = {0.0f, 0.0f};

        float movementSpeed = 1.0f;
        float turnSpeed = 0.1f;

        float moveTransitionEffect = 0.5f;
        float maxSpeed = 50.0f;
        float cameraDrag = 0.975f;

        glm::vec3 velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    };
}
