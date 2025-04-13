#include "camera_controller.h"

#include <algorithm>
#include <iostream>
#include <ostream>

#include "input.h"
#include "GLFW/glfw3.h"

namespace Raytracing
{
    void CameraController::Update(float deltaTime)
    {
        if (!camera) return;

        m_IsCameraMoving =
            Input::IsKeyPressed(GLFW_KEY_W) ||
            Input::IsKeyPressed(GLFW_KEY_S) ||
            Input::IsKeyPressed(GLFW_KEY_D) ||
            Input::IsKeyPressed(GLFW_KEY_A) ||
            Input::IsKeyPressed(GLFW_KEY_SPACE) ||
            Input::IsKeyPressed(GLFW_KEY_LEFT_CONTROL);

        float speed;
        glm::vec3 newVel = {0.0f, 0.0f, 0.0f};

        if (!m_IsCameraMoving)
        {
            speed = maxSpeed * deltaTime;
            newVel = -velocity;
        }
        else
        {
            moveTransitionEffect += 0.75f * deltaTime;
            moveTransitionEffect = std::min(moveTransitionEffect, 1.0f);
            speed = moveTransitionEffect * movementSpeed * deltaTime;

            if (Input::IsKeyPressed(GLFW_KEY_W)) newVel += camera->GetForwardVector();
            if (Input::IsKeyPressed(GLFW_KEY_S)) newVel += -camera->GetForwardVector();
            if (Input::IsKeyPressed(GLFW_KEY_D)) newVel += camera->GetRightVector();
            if (Input::IsKeyPressed(GLFW_KEY_A)) newVel += -camera->GetRightVector();
            if (Input::IsKeyPressed(GLFW_KEY_SPACE)) newVel += glm::vec3(0.0f, 1.0f, 0.0f);
            if (Input::IsKeyPressed(GLFW_KEY_LEFT_CONTROL)) newVel += -glm::vec3(0.0f, 1.0f, 0.0f);
            if (Input::IsKeyPressed(GLFW_KEY_LEFT_SHIFT)) speed *= 2.0f;
        }
        speed = std::clamp(speed, 0.0f, maxSpeed);
        velocity = cameraDrag * velocity + (newVel * speed);

        camera->GetTransform().m_Position += velocity;


        const glm::vec2 mouseInput = {
            Input::GetMousePosX(),
            Input::GetMousePosY()
        };
        mouseDelta = lastMousePos - mouseInput;
        lastMousePos = mouseInput;

        if (Input::IsMousePressed(GLFW_MOUSE_BUTTON_RIGHT))
        {
            mouseDelta *= turnSpeed;

            camera->GetTransform().m_Rotation.y += mouseDelta.x;
            camera->GetTransform().m_Rotation.x += mouseDelta.y;

            camera->GetTransform().m_Rotation.x = std::clamp(camera->GetTransform().m_Rotation.x, -89.0f, 89.0f);

            mouseDelta = {0.0f, 0.0f};
        }

        camera->Update();
    }
}
