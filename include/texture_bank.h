#pragma once

#include <cstdint>
#include <array>
#include <filesystem>
#include <vector>

namespace wall_textures {

struct Rgb {
    std::uint8_t r, g, b;
};

std::array<Rgb, 256> loadPalette(const std::filesystem::path& path);

struct Texture {
    int width = 0, height = 0;
    std::vector<Rgb> pixels; // Row-major RGB, expanded from the external palette.
};

struct Bank {
    std::vector<Texture> textures;
};

struct PlaneBank {
    Texture floor;
    Texture ceiling;
};

// SOL1: 128 rows, each containing 128 floor indices followed by 128 ceiling indices.
PlaneBank loadPlanes(const std::filesystem::path& dataDirectory, std::uint16_t solBank,
                     std::uint16_t palBank);

// Reads DALLE<n>.DAL and PAL<n>.PAL without SDL, PNGs or a GAME.LAB dependency.
// Only the researched DALLE1 layout is supported; throws on unsupported/invalid data.
Bank load(const std::filesystem::path& dataDirectory, std::uint16_t dalleBank,
          std::uint16_t palBank);

// The 20 validated ADALLE.SPR overlays, in file order, using the same palette.
Bank loadWallOverlays(const std::filesystem::path& dataDirectory, std::uint16_t palBank);

} // namespace wall_textures
