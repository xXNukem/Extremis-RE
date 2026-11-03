#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace spr_format {
struct Image {
    int width, height, anchorX, anchorY;
    std::vector<std::uint8_t> indices;
};

// Shared OBJET/ADALLE container reader, without palette or transparency policy.
std::vector<Image> decode(const std::vector<std::uint8_t>& bytes);
std::vector<Image> load(const std::filesystem::path& path);
} // namespace spr_format
