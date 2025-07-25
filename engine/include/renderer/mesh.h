﻿#pragma once

#include <array>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <vulkan/vulkan_core.h>

#include <util/utils.h>

namespace MongooseVK
{
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec3 color;
        glm::vec2 texCoord;
        glm::vec3 tangent = glm::vec3(0.0f);
        glm::vec3 bitangent = glm::vec3(0.0f);
        uint32_t padding = 0;

        bool operator==(const Vertex& other) const
        {
            return pos == other.pos
                   && normal == other.normal
                   && color == other.color
                   && texCoord == other.texCoord
                   && tangent == other.tangent
                   && bitangent == other.bitangent;
        }
    };

    struct VulkanVertex : Vertex {
        static VkVertexInputBindingDescription GetBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};

            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 6> GetAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 6> attributeDescriptions{};

            // Vertex position
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);

            // Vertex color
            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, color);

            // Vertex UV coord
            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

            // Vertex normal
            attributeDescriptions[3].binding = 0;
            attributeDescriptions[3].location = 3;
            attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[3].offset = offsetof(Vertex, normal);

            // Vertex tangent
            attributeDescriptions[4].binding = 0;
            attributeDescriptions[4].location = 4;
            attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[4].offset = offsetof(Vertex, tangent);

            // Vertex bitangent
            attributeDescriptions[5].binding = 0;
            attributeDescriptions[5].location = 5;
            attributeDescriptions[5].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[5].offset = offsetof(Vertex, bitangent);

            return attributeDescriptions;
        }
    };

    namespace Primitives
    {
        const std::vector<uint32_t> RECTANGLE_INDICES = {
            0, 1, 2, 2, 3, 0
        };

        const std::vector<Vertex> RECTANGLE_VERTICES = {
            {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f},},
            {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f},},
            {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f},},
            {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f},}
        };

        const std::vector<uint32_t> GRID_INDICES = {
            0, 2, 1, 2, 0, 3
        };

        const std::vector<Vertex> GRID_VERTICES = {
            {{-1.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f},},
            {{1.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f},},
            {{1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f},},
            {{-1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f},}
        };

        const std::vector<uint32_t> CUBE_INDICES = {
            3, 0, 2,
            0, 3, 1,

            7, 6, 4,
            4, 5, 7,

            8, 9, 11,
            11, 10, 8,

            13, 14, 12,
            14, 13, 15,

            16, 17, 19,
            19, 18, 16,

            21, 22, 20,
            22, 21, 23
        };

        const std::vector<Vertex> CUBE_VERTICES = {
            {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // top-right              0
            {{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // top-left              1
            {{1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}, // bottom-right          2
            {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}, // bottom-left          3

            {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // top-right              4
            {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // top-left              5
            {{1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}, // bottom-right          6
            {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}, // bottom-left          7

            {{-1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}, // top-right              8
            {{-1.0f, 1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // top-left              9
            {{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}, // bottom-right          10
            {{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // bottom-left          11

            {{1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // top-right              8
            {{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}, // top-left              9
            {{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // bottom-right          10
            {{1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}, // bottom-left          11

            {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // top-right              8
            {{1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // top-left              9
            {{-1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}, // bottom-right          10
            {{1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}, // bottom-left          11

            {{1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // top-right              8
            {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // top-left              9
            {{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}, // bottom-right          10
            {{-1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}, // bottom-left          11
        };
    }
}

namespace std
{
    template<>
    struct hash<MongooseVK::Vertex> {
        size_t operator()(MongooseVK::Vertex const& vertex) const noexcept
        {
            size_t seed = 0;
            hashCombine(seed, vertex.pos, vertex.color, vertex.normal, vertex.texCoord);
            return seed;
        }
    };
}
