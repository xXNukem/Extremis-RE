#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace wall_textures {

struct Rgb {
    std::uint8_t r, g, b;
};

struct Texture {
    int width = 0, height = 0;
    std::vector<Rgb> pixels; // Row-major RGB, expanded from the external palette.
};

struct Bank {
    std::vector<Texture> textures;
};

// Reads DALLE<n>.DAL and PAL<n>.PAL without SDL, PNGs or a GAME.LAB dependency.
// Only the researched DALLE1 layout is supported; throws on unsupported/invalid data.
Bank load(const std::filesystem::path& dataDirectory, std::uint16_t dalleBank,
          std::uint16_t palBank);

} // namespace wall_textures
