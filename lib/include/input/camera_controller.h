#pragma once
#include "renderer/camera.h"
#include "util/core.h"

namespace MongooseVK
{
    class CameraController {
    public:
        CameraController() = default;
        ~CameraController() = default;

        void Update(float deltaTime);
        void SetCamera(const Ref<Camera>& _camera) { camera = _camera; }

    public:
        float movementSpeed = 15.0f;
        float turnSpeed = 0.1f;
        float maxSpeed = 50.0f;

    private:
        Ref<Camera> camera;

        bool m_IsCameraMoving = false;
        glm::vec2 mouseDelta = {0.0f, 0.0f};
        glm::vec2 lastMousePos = {0.0f, 0.0f};
    };
}
