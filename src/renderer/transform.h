#pragma once

#include <glm/glm.hpp>
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/quaternion.hpp>

struct Transform {
    glm::vec3 m_Position = glm::vec3(0.0f);
    glm::vec3 m_Scale = glm::vec3(1.0f);
    glm::vec3 m_Rotation = glm::vec3(0.0f);
    glm::quat m_Rotation_Quat = glm::quat();

    glm::mat4 GetTransform() const
    {
        return translate(glm::mat4(1.0f), m_Position) * toMat4(glm::quat(radians(m_Rotation))) * scale(glm::mat4(1.0f), m_Scale);
    }

    glm::mat4 GetTransform2() const
    {
        return translate(glm::mat4(1.0f), m_Position) * toMat4(m_Rotation_Quat) * scale(glm::mat4(1.0f), m_Scale);
    }

    glm::vec3 GetForwardDirection() const
    {
        return GetTransform() * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
    }

    glm::mat4 GetNormalMatrix() const
    {
        return inverseTranspose(GetTransform());
    }
};
