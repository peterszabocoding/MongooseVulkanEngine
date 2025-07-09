#pragma once
#include "camera.h"
#include "texture_atlas.h"

#include "glm/vec3.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

namespace MongooseVK
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

    struct Cascade {
        float splitDepth;
        glm::mat4 viewProjMatrix;
    };

    struct Light {
        glm::vec3 color = glm::vec3(1.0f);
        float intensity = 20.0f;
        float bias = 0.05f;

        ShadowType shadowType = ShadowType::NONE;
        ShadowMapResolution shadowMapResolution = ShadowMapResolution::ULTRA_HIGH;
        TextureAtlas::AtlasBox* shadowMapRegion = nullptr;
    };

    constexpr size_t SHADOW_MAP_CASCADE_COUNT = 4;

    struct DirectionalLight : Light {
        glm::vec3 direction = normalize(glm::vec3(0.0f, -1.0f, 0.0f));
        glm::vec3 center = glm::vec3(0.0f);
        float ambientIntensity = 1.0f;
        float cascadeSplitLambda = 0.85f;

        Cascade cascades[SHADOW_MAP_CASCADE_COUNT];

        void UpdateCascades(const Camera& camera)
        {
            float cascadeSplits[SHADOW_MAP_CASCADE_COUNT];

            const float nearClip = camera.GetNearPlane();
            const float farClip = camera.GetFarPlane();
            const float clipRange = farClip - nearClip;

            const float minZ = nearClip;
            const float maxZ = nearClip + clipRange;

            const float range = maxZ - minZ;
            const float ratio = maxZ / minZ;

            // Calculate split depths based on view camera frustum
            // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
            for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
            {
                const float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
                const float log = minZ * std::pow(ratio, p);
                const float uniform = minZ + range * p;
                const float d = cascadeSplitLambda * (log - uniform) + uniform;
                cascadeSplits[i] = (d - nearClip) / clipRange;
            }

            // Calculate orthographic projection matrix for each cascade
            float lastSplitDist = 0.0;
            for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
            {
                const float splitDist = cascadeSplits[i];

                glm::vec3 frustumCorners[8] = {
                    glm::vec3(-1.0f, 1.0f, 0.0f),
                    glm::vec3(1.0f, 1.0f, 0.0f),
                    glm::vec3(1.0f, -1.0f, 0.0f),
                    glm::vec3(-1.0f, -1.0f, 0.0f),
                    glm::vec3(-1.0f, 1.0f, 1.0f),
                    glm::vec3(1.0f, 1.0f, 1.0f),
                    glm::vec3(1.0f, -1.0f, 1.0f),
                    glm::vec3(-1.0f, -1.0f, 1.0f),
                };

                // Project frustum corners into world space
                glm::mat4 invCam = inverse(camera.GetProjection() * camera.GetView());
                for (uint32_t j = 0; j < 8; j++)
                {
                    glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[j], 1.0f);
                    frustumCorners[j] = invCorner / invCorner.w;
                }

                for (uint32_t j = 0; j < 4; j++)
                {
                    glm::vec3 dist = frustumCorners[j + 4] - frustumCorners[j];
                    frustumCorners[j + 4] = frustumCorners[j] + (dist * splitDist);
                    frustumCorners[j] = frustumCorners[j] + (dist * lastSplitDist);
                }

                // Get frustum center
                glm::vec3 frustumCenter = glm::vec3(0.0f);
                for (uint32_t j = 0; j < 8; j++)
                {
                    frustumCenter += frustumCorners[j];
                }
                frustumCenter /= 8.0f;

                float radius = 0.0f;
                for (uint32_t j = 0; j < 8; j++)
                {
                    const float distance = length(frustumCorners[j] - frustumCenter);
                    radius = glm::max(radius, distance);
                }
                radius = std::ceil(radius * 16.0f) / 16.0f;

                glm::vec3 maxExtents = glm::vec3(radius);
                glm::vec3 minExtents = -maxExtents;

                glm::mat4 lightViewMatrix = lookAt(frustumCenter - direction * -minExtents.z, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
                glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 3.0f * minExtents.z,
                                                        maxExtents.z - minExtents.z);

                // Store split distance and matrix in cascade
                cascades[i].splitDepth = (camera.GetNearPlane() + splitDist * clipRange) * -1.0f;
                cascades[i].viewProjMatrix = lightOrthoMatrix * lightViewMatrix;

                lastSplitDist = cascadeSplits[i];
            }
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
