#pragma once

#include "texture_bank.h"

namespace plane_material {
struct Texel { int x, y; };
// Original SOL world-aligned repeat period: 400 units in X and Y, increasing U/V.
Texel coordinates(double worldX, double worldY, int width, int height);
const wall_textures::Texture& select(const wall_textures::PlaneBank& bank, bool ceiling);
} // namespace plane_material
