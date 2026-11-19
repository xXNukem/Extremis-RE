#pragma once

#include "vortex/render/first_person/camera_pose.h"
#include "vortex/render/first_person/planes/plane_material.h"

#include <optional>
#include <utility>
#include <vector>

namespace vortex::planes {
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
std::optional<Row> projectRow(const CameraPose& camera, const Projection& projection, int y);
// VORTEX_HIDDEN_PLANE_CULLING_V1: skip spans already covered by opaque wall columns.
// Each pair is [first covered Y, one-past-last covered Y] for one X.
// nullptr retains the historical, unmasked rendering behavior.
// A nonpositive or non-finite repeat period leaves the framebuffer untouched.
void render(std::vector<std::uint32_t>& pixels, const CameraPose& camera,
            const Projection& projection, const PlaneBank& materials, double repeatPeriod,
            const std::vector<std::pair<int,int>>* wallCoverage = nullptr);
} // namespace vortex::planes
