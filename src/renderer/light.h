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
        glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
        glm::vec3 ambientColor = glm::vec3(1.0f);
        float ambientIntensity = 0.1f;

        static glm::mat4 GetProjection()
        {
            return glm::ortho(-80.0f, 80.0f, -80.0f, 80.0f, -80.0f, 80.0f);
        }

        glm::mat4 GetTransform() const
        {
            return GetProjection() * lookAt(direction, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
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
