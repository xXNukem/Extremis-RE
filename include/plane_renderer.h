#pragma once

#include "camera.h"
#include "texture_bank.h"

#include <optional>

namespace planes {
struct Projection {
    int width, height;
    double focal;
    double eyeHeight;
    double ceilingHeight;
};
struct Row {
    double x, y;
    double stepX, stepY;
    double depth;
    bool ceiling;
};

// World position of first pixel center and constant step across one horizontal row.
std::optional<Row> projectRow(const Camera& camera, const Projection& projection, int y);
// Overlays valid horizontal materials on an existing flat background, before walls.
void render(std::vector<std::uint32_t>& pixels, const Camera& camera,
            const Projection& projection, const wall_textures::PlaneBank& materials);
} // namespace planes
