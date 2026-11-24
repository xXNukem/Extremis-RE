#pragma once

#include <cstddef>
#include <vector>
#include "vortex/core/math/constants.h"
#include "extremis/gameplay/player/camera.h"
#include "extremis/resources/game_lab.h"
#include "extremis/world/door.h"
#include "vortex/render/first_person/raycast/static_ray_grid.h"
#include "vortex/render/first_person/scene_renderer.h"

namespace extremis::render_detail {
using vortex::Point;
using vortex::ProjectedColumn;

using vortex::math::pi;
inline constexpr double wallHeight = 256.0;
inline constexpr double fov = Camera::horizontalFov;

using CameraProjection = vortex::first_person::ColumnProjection;
using vortex::first_person::columnRay;

vortex::PreparedRayScene prepareRayScene(const game_lab::Level& level, const Camera& camera,
                                         const std::vector<doors::Door>* doors);

class StaticRayGrid {
    const game_lab::Level* level_;

public:
    explicit StaticRayGrid(const game_lab::Level& level, bool force = false);

    bool matches(const game_lab::Level& level) const;
    bool valid() const;
    std::size_t bytes() const;
    double backtrack(const Camera& camera) const;
    vortex::RayHit cast(const vortex::PreparedRayScene& scene, const Camera& camera, Point direction,
                       double backtrack, unsigned* visitedCells = nullptr,
                       std::size_t* testedWalls = nullptr);

    vortex::StaticRayGrid index;
};

} // namespace extremis::render_detail
