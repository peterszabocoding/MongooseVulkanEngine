#pragma once
#include "renderer/camera.h"
#include "util/core.h"

namespace MongooseVK
{
    class CameraController {
    public:
        CameraController(Camera& _camera): camera(_camera) {}
        ~CameraController() = default;

        void Update(float deltaTime);

    public:
        float movementSpeed = 15.0f;
        float turnSpeed = 0.1f;
        float maxSpeed = 50.0f;

    private:
        Camera& camera;

        bool m_IsCameraMoving = false;
        glm::vec2 mouseDelta = {0.0f, 0.0f};
        glm::vec2 lastMousePos = {0.0f, 0.0f};
    };
}
