#pragma once

#include "vortex/core/graphics/texture.h"

#include <utility>

namespace vortex {
struct ProjectedColumn {
    float top, bottom;
    double depth;
    double unclippedTop, unclippedBottom;
};

ProjectedColumn projectWall(int height, double focal, double eyeHeight,
                            double wallHeight, double distance);

double distanceShade(double depth);

std::uint32_t rgbaPixel(Rgb color, double shade = 1.0);

// The caller resolves a valid texture column or supplies a flat base color.
// A depth buffer, when present, receives the unclamped hit distance.
// Returns the half-open range of rows covered by the wall.
std::pair<int, int> rasterizeWallColumn(
    std::vector<std::uint32_t>& pixels, int width, int height, int column,
    double wallDistance, const ProjectedColumn& projected, const Texture* texture,
    int textureX, Rgb baseColor, std::vector<double>* depth = nullptr);

} // namespace vortex
