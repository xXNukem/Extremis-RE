#include "vortex/render/first_person/walls/wall_raster.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace vortex {
ProjectedColumn projectWall(int height, double focal, double eyeHeight,
                            double wallHeight, double distance)
{
    const double depth = std::max(0.01, distance);
    const double unclippedTop = height * 0.5 -
        (wallHeight - eyeHeight) * focal / depth;
    const double unclippedBottom = height * 0.5 +
        eyeHeight * focal / depth;
    const float top = float(std::clamp(unclippedTop, 0.0, double(height)));
    const float bottom = float(std::clamp(unclippedBottom, 0.0, double(height)));
    return {top, bottom, depth, unclippedTop, unclippedBottom};
}

double distanceShade(double depth)
{
    return 0.2 + 0.8 / (1.0 + depth / 700.0);
}

std::uint32_t rgbaPixel(Rgb color, double shade)
{
    return (std::uint32_t(color.r * shade) << 24) |
           (std::uint32_t(color.g * shade) << 16) |
           (std::uint32_t(color.b * shade) << 8) | 255;
}

std::pair<int, int> rasterizeWallColumn(
    std::vector<std::uint32_t>& pixels, int width, int height, int column,
    double wallDistance, const ProjectedColumn& projected, const Texture* texture,
    int textureX, Rgb baseColor, std::vector<double>* depth)
{
    const double projectedHeight = projected.unclippedBottom - projected.unclippedTop;
    const double shade = distanceShade(projected.depth);
    const int begin = int(std::ceil(std::clamp(projected.unclippedTop, 0.0,
                                              double(height)) - 0.5));
    const int end = int(std::ceil(std::clamp(projected.unclippedBottom, 0.0,
                                            double(height)) - 0.5));
    const auto flatPixel = rgbaPixel(baseColor, shade);
    // VORTEX_WALL_RASTER_FASTPATH_V1: a column reuses the same 74 source texels
    // many times at high resolution. Shade each texel once, not each output pixel.
    // The original path remains for short projected walls or other texture sizes.
    constexpr int cachedRows = 74;
    std::array<std::uint32_t, cachedRows> shadedTexels;
    const bool cachedShade = texture && texture->height == cachedRows && end - begin > cachedRows;
    if (cachedShade) {
        for (int texelY = 0; texelY < cachedRows; ++texelY) {
            const auto color = texture->pixels[std::size_t(texelY) * texture->width + textureX];
            shadedTexels[texelY] = rgbaPixel(color, shade);
        }
    }
    // Full-height walls hide billboard overhang above or below the raster bounds.
    if (depth) {
        for (int y = 0; y < height; ++y)
            (*depth)[std::size_t(y) * width + column] = wallDistance;
    }
    if (cachedShade) {
        // Use the exact clipped-column sampler with a fixed texture height,
        // no per-pixel material branch, and a single stride increment.
        const auto* colors = shadedTexels.data();
        auto* output = pixels.data();
        auto offset = std::size_t(begin) * width + column;
        for (int y = begin; y < end; ++y, offset += width) {
            const double v = (y + 0.5 - projected.unclippedTop) / projectedHeight;
            const int texelY = int(v * cachedRows);
            const int row = texelY < 0 ? 0 : texelY >= cachedRows ? cachedRows - 1 : texelY;
            output[offset] = colors[row];
        }
        return {begin, end};
    }
    for (int y = begin; y < end; ++y) {
        auto pixel = flatPixel;
        if (texture) {
            // Use the original projected extent: clipping crops rather than stretching.
            const double v = (y + 0.5 - projected.unclippedTop) / projectedHeight;
            const int textureY = std::clamp(int(v * texture->height), 0, texture->height - 1);
            const auto color = texture->pixels[std::size_t(textureY) * texture->width + textureX];
            pixel = rgbaPixel(color, shade);
        }
        pixels[std::size_t(y) * width + column] = pixel;
    }
    return {begin, end};
}
} // namespace vortex
