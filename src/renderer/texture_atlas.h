#pragma once
#include "glm/vec2.hpp"

#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <string>

class TextureAtlas
{

public:

    struct AtlasBox
    {
        glm::uvec2 topLeft;
        glm::uvec2 bottomRight;
    };

    void Clear();
    void AddTexture(const uint16_t textureSize)
    {
        m_TextureSizes.push_back(textureSize);

    }
    void AllocateTextureAtlas(glm::uvec2 const& atlasSize);

    AtlasBox* GetTextureRegion(const uint16_t size)
    {
        if (m_Boxes[size].empty())
        {
            throw std::runtime_error("TextureAtlas: Atlas ran out of texture of this size: " + std::to_string(size));
        }

        AtlasBox* region = &m_Boxes[size].back();
        m_Boxes[size].pop_back();
        return region;
    }

public:
    std::vector<uint16_t> m_TextureSizes;
    std::unordered_map<uint16_t, std::vector<AtlasBox>> m_Boxes;
};