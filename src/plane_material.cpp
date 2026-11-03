#include "plane_material.h"

#include <cmath>
#include <algorithm>

namespace plane_material {
namespace {
int coordinate(double world, int size)
{
    constexpr double period = 400.0;
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
