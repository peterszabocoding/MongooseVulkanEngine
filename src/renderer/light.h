#pragma once
#include "texture_atlas.h"

#include "glm/vec3.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

namespace Raytracing
{
    enum class ShadowType: uint8_t {
        NONE = 0,
        HARD = 1,
        SOFT = 2
    };

    enum class ShadowMapResolution : uint16_t {
        ULTRA_LOW = 128,
        LOW = 256,
        MEDIUM = 512,
        HIGH = 1024,
        VERY_HIGH = 2048,
        ULTRA_HIGH = 4096
    };

    struct Light {
        glm::vec3 color = glm::vec3(1.0f);
        float intensity = 1.0f;
        float shadowBias = 0.0005f;
        ShadowType shadowType = ShadowType::NONE;
        ShadowMapResolution shadowMapResolution = ShadowMapResolution::MEDIUM;
        TextureAtlas::AtlasBox* shadowMapRegion = nullptr;
    };

    struct DirectionalLight : Light {
        glm::vec3 direction = normalize(glm::vec3(0.0f, -1.0f, 0.0f));
        glm::vec3 color = glm::vec3(1.0f);
        float ambientIntensity = 0.35f;
        float intensity = 1.0f;
        float nearPlane = 1.0f;
        float farPlane = 50.0f;
        float orthoSize = 20.0f;
        float bias = 0.05f;

        glm::mat4 GetProjection() const
        {
            return glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);
        }

        glm::mat4 GetView() const
        {
            // We need a position for the "camera" looking along the light direction.
            // A simple trick is to choose an arbitrary world-space origin and look along -lightDir.
            glm::vec3 lightPosition = -2.0f * 20.0f * direction; // Offset to see the scene
            glm::vec3 upDirection = glm::vec3(0.0f, 1.0f, 0.0f); // Choose an appropriate up vector

            // Handle the case where the light direction is almost parallel to the up direction
            if (glm::abs(glm::dot(direction, upDirection)) > 0.99f) {
                upDirection = glm::vec3(0.0f, 0.0f, 1.0f); // Use a different up vector
            }

            return lookAt(lightPosition, glm::vec3(0.0f), upDirection);
            // lookAt(normalize(direction), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        }

        glm::mat4 GetTransform() const
        {
            return GetProjection() * GetView();
        }
    };

    struct PointLight : Light {
        glm::vec3 position = glm::vec3(0.0f);
        float attenuationRadius = 10.0f;

        glm::mat4 GetProjection() const
        {
            return glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, attenuationRadius * 1.5f);
        }

        [[nodiscard]] std::vector<glm::mat4> GetTransform() const
        {
            const glm::mat4 shadowProj = GetProjection();
            std::vector shadowTransforms =
            {
                shadowProj * lookAt(position, position + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)),
                shadowProj * lookAt(position, position + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)),
                shadowProj * lookAt(position, position + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)),
                shadowProj * lookAt(position, position + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)),
                shadowProj * lookAt(position, position + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)),
                shadowProj * lookAt(position, position + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0))
            };
            return shadowTransforms;
        }
    };

    struct SpotLight : PointLight {
        glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
        float attenuationAngle = 0.75f;

        glm::mat4 GetProjection() const
        {
            return glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, attenuationRadius * 1.5f);
        }

        glm::mat4 GetTransform() const
        {
            return GetProjection() * lookAt(position, position - direction, glm::vec3(0.0f, 1.0f, 0.0f));
        }
    };
}
