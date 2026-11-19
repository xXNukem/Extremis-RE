#include "plane_material.h"

#include <cmath>
#include <cstdint>
#include <algorithm>

namespace plane_material {
namespace {
int coordinate(double world, int size)
{
    constexpr double period = 400.0;
    // VORTEX_PLANE_UV_FASTPATH_V1: SOL textures are 128x128. Index the
    // repeating texel directly instead of performing fmod for every pixel.
    // Fall back at texel boundaries so rounding and the original seam fix
    // remain pixel-identical, including for negative world coordinates.
    if (size == 128 && std::abs(world) < 1000000.0) {
        const double scaled = world * (128.0 / period);
        const auto cell = static_cast<std::int64_t>(std::floor(scaled));
        const double fraction = scaled - double(cell);
        if (fraction > 1e-9 && fraction < 1.0 - 1e-9)
            return int(static_cast<std::uint64_t>(cell) & 127u);
    }
    // Historical path: non-SOL sizes, exceptional coordinates and edges.
    double wrapped = std::fmod(world, period);
    if (wrapped < 0) wrapped += period;
    // Pixel-ray cancellation at a tile edge can leave a tiny negative coordinate.
    if (wrapped < 1e-9 || wrapped > period - 1e-9) wrapped = 0;
    return std::clamp(int(wrapped / period * size), 0, size - 1);
}
} // namespace

Texel coordinates(double worldX, double worldY, int width, int height)
{
    if (width <= 0 || height <= 0 || !std::isfinite(worldX) || !std::isfinite(worldY)) return {0, 0};
    return {coordinate(worldX, width), coordinate(worldY, height)};
}

const wall_textures::Texture& select(const wall_textures::PlaneBank& bank, bool ceiling)
{
    return ceiling ? bank.ceiling : bank.floor;
}
} // namespace plane_material
