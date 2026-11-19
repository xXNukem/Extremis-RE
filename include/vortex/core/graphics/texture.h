#pragma once

#include "vortex/core/graphics/color.h"

#include <cstddef>
#include <vector>

namespace vortex {
struct Texture {
    int width = 0, height = 0;
    std::vector<Rgb> pixels; // Row-major RGB.
};

// Preserve inlining at per-column material selection and row sampling.
inline bool validTexture(const Texture& texture)
{
    return texture.width > 0 && texture.height > 0 &&
           texture.pixels.size() == std::size_t(texture.width) * texture.height;
}
} // namespace vortex
