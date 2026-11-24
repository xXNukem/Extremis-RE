#include <vector>
#include "extremis/render/first_person/scene/ray_queries.h"
#include "extremis/resources/sector_coordinates.h"

#include "ray_scene.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

namespace {
using vortex::Point;
using vortex::SegmentHit;
using vortex::cross;

std::optional<SegmentHit> intersectSegment(const Camera& camera, Point direction,
                                       Point a, Point b)
{
    const Point edge{b.x - a.x, b.y - a.y};
    const Point delta{a.x - camera.x, a.y - camera.y};
    const double denominator = cross(direction, edge);
    if (std::abs(denominator) < 1e-9) {
        return std::nullopt; // Parallel ray or zero-length segment.
    }
    const double t = cross(delta, edge) / denominator;
    const double u = cross(delta, direction) / denominator;
    if (t > 1e-6 && u >= -1e-9 && u <= 1 + 1e-9) {
        return SegmentHit{t, std::clamp(u, 0.0, 1.0)};
    }
    return std::nullopt;
}

} // namespace

WallHit castWallRay(const game_lab::Level& level, const Camera& camera,
                    double dx, double dy)
{
    WallHit hit{std::numeric_limits<double>::infinity(), 0};
    bool hitFacesCamera = false;
    for (const auto& sector : level.sectors) {
        for (const auto& wall : sector.walls) {
            if ((wall.type & 0xf000) == 0x1000) {
                continue; // Portals are not visible surfaces in this prototype.
            }
            const auto a = game_lab::worldVertex(sector, wall.vertexA);
            const auto b = game_lab::worldVertex(sector, wall.vertexB);
            const auto intersection = intersectSegment(camera, {dx, dy}, a, b);
            if (!intersection) continue;
            const bool facesCamera = cross({b.x - a.x, b.y - a.y},
                                           {camera.x - a.x, camera.y - a.y}) < 0;
            // Shared boundaries can have opposite faces with different segment
            // cuts. Roundoff must not choose alternating materials/U directions.
            // Use the original A-left/B-right convention only to resolve ties;
            // isolated walls retain their existing two-sided rendering.
            const bool coincident = std::isfinite(hit.distance) &&
                std::abs(intersection->distance - hit.distance) <=
                    1e-9 * std::max(1.0, std::abs(hit.distance));
            if (coincident ? (facesCamera && !hitFacesCamera)
                           : intersection->distance < hit.distance) {
                hit = {intersection->distance, wall.type, intersection->position};
                hitFacesCamera = facesCamera;
            }
        }
    }
    return hit;
}

WallHit castSceneRay(const game_lab::Level& level, const Camera& camera,
                    double dx, double dy, const std::vector<doors::Door>* doors)
{
    auto hit = castWallRay(level, camera, dx, dy);
    if (!doors) return hit;
    for (const auto& door : *doors) {
        for (const auto& closedFace : door.faces) {
            const auto face = doors::visibleFace(level, door, closedFace);
            if (!face) continue;
            const Point a{face->a.x, face->a.y}, b{face->b.x, face->b.y};
            // Original projected walls retain A-left/B-right faces only.
            if (cross({b.x - a.x, b.y - a.y}, {camera.x - a.x, camera.y - a.y}) >= 0)
                continue;
            const auto intersection = intersectSegment(camera, {dx, dy}, a, b);
            if (intersection && intersection->distance < hit.distance) {
                hit = {intersection->distance, face->material, intersection->position,
                       face->textureSpan};
            }
        }
    }
    return hit;
}
