#pragma once

#include "camera.h"
#include "texture_bank.h"

#include <optional>
#include <utility>
#include <vector>

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
// VORTEX_HIDDEN_PLANE_CULLING_V1: skip spans already covered by opaque wall columns.
// Each pair is [first covered Y, one-past-last covered Y] for one X.
// nullptr retains the historical, unmasked rendering behavior.
void render(std::vector<std::uint32_t>& pixels, const Camera& camera,
            const Projection& projection, const wall_textures::PlaneBank& materials,
            const std::vector<std::pair<int,int>>* wallCoverage = nullptr);
} // namespace planes
