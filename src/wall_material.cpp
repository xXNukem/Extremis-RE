#include "wall_material.h"

#include <algorithm>

std::optional<std::size_t> wallTextureIndex(std::uint16_t type, std::size_t textureCount)
{
    const auto family = type & 0xf000;
    if (family == 0x1000 || family > 0x3000) {
        return std::nullopt;
    }
    // The original clears AH, increments for its drawing API, then decrements
    // before multiplying by 4440. The effective file index is the low byte.
    const std::size_t index = type & 0xff;
    return index < textureCount ? std::optional<std::size_t>(index) : std::nullopt;
}

int wallTextureColumnCount(std::uint16_t type, int textureWidth)
{
    return (type & 0xf000) == 0x2000 && textureWidth > 0
        ? std::max(1, textureWidth / 2) : textureWidth;
}
