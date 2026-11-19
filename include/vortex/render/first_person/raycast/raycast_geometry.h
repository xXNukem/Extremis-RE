#pragma once

#include "vortex/core/geometry/point.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace vortex {
struct SegmentHit {
    double distance, position;
};

struct RayHit {
    double distance; // Ray parameter; perpendicular depth for camera-plane rays.
    std::uint16_t type; // Opaque material ID supplied by the caller.
    double segmentPosition = 0; // Fraction from vertex A to B, clamped to [0, 1].
    double textureSpan = 1.0; // Active fraction of material U; 1 for static walls.
};

constexpr double rayParallelTolerance = 1e-9;
constexpr double rayEndpointTolerance = 1e-9;
constexpr double rayTieTolerance = 1e-9;

// VORTEX_RAYCAST_PRECOMPUTE_V1: prepare once for each origin and surface state,
// then reuse for all rays from that origin.
struct PreparedRaySurface {
    Point delta;
    Point edge;
    double numerator;
    std::uint16_t material;
    double textureSpan;
    bool facesCamera;
};

struct PreparedRayScene {
    std::vector<PreparedRaySurface> walls;
    // Already filtered by the caller. A dynamic hit can only override a
    // strictly farther static or dynamic hit.
    std::vector<PreparedRaySurface> dynamicSurfaces;
};

PreparedRaySurface prepareRaySurface(Point a, Point b, Point camera,
                                     std::uint16_t material, double textureSpan);

std::optional<SegmentHit> intersectPreparedRay(const PreparedRaySurface& surface,
                                               Point direction);

void considerWallHit(RayHit& hit, bool& hitFacesCamera, const PreparedRaySurface& wall,
                     const SegmentHit& intersection);

RayHit castDynamicSurfaces(const PreparedRayScene& scene, Point direction, RayHit hit);

RayHit castPreparedSceneRay(const PreparedRayScene& scene, Point direction);

} // namespace vortex
