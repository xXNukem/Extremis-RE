#pragma once

#include "vortex/core/graphics/texture.h"

#include <cstddef>

namespace vortex::sprites {
struct Image {
    Texture texture;
    int anchorX = 0, anchorY = 0;
    std::vector<std::uint8_t> indices; // Index 0 alone is transparent, regardless of RGB.
};

struct Bank {
    std::vector<Image> images;
};

struct Instance {
    double x, y, z, height;
    std::size_t image;
    bool flipX = false, flipY = false;
    bool wallImpact = false; // Surface effect; ordinary sprites retain normal depth testing.
    // Surface overlay for the NoNaN path with a wall-only column depth limit.
    bool overlaySpriteAtEqualDepth = false;
};

}
