#pragma once

#include "vortex/core/graphics/texture.h"

namespace vortex {
struct PlaneBank {
    Texture floor;
    Texture ceiling;
};

namespace plane_material {
struct Texel { int x, y; };
// World-aligned repeat period in X and Y. Invalid dimensions, period or
// position return the origin texel.
Texel coordinates(double worldX, double worldY, int width, int height, double repeatPeriod);
const Texture& select(const PlaneBank& bank, bool ceiling);
} // namespace plane_material
} // namespace vortex
