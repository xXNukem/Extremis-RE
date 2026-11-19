#include "extremis/render/first_person/camera/camera_start.h"
#include "extremis/resources/sector_coordinates.h"
#include "vortex/core/math/constants.h"
#include "extremis/render/first_person/scene/ray_queries.h"

#include "../scene/ray_scene.h"
#include "extremis/world/player_collision.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {
using extremis::render_detail::pi;
using vortex::Point;
using vortex::cross;

struct PolygonBounds {
    Point low{1e30, 1e30};
    Point high{-1e30, -1e30};
    double area2 = 0;
};

double segmentDistance(Point point, Point a, Point b)
{
    const double dx = b.x - a.x, dy = b.y - a.y;
    const double length2 = dx * dx + dy * dy;
    const double t = length2 == 0 ? 0 : std::clamp(
        ((point.x - a.x) * dx + (point.y - a.y) * dy) / length2, 0.0, 1.0);
    return std::hypot(point.x - a.x - t * dx, point.y - a.y - t * dy);
}

PolygonBounds polygonBounds(const game_lab::Sector& sector,
                            const game_lab::Polygon& polygon)
{
    PolygonBounds bounds;
    for (std::size_t i = 0; i < polygon.vertices.size(); ++i) {
        const auto a = game_lab::worldVertex(sector, polygon.vertices[i]);
        const auto b = game_lab::worldVertex(sector, polygon.vertices[(i + 1) % polygon.vertices.size()]);
        bounds.area2 += cross(a, b);
        bounds.low.x = std::min(bounds.low.x, a.x);
        bounds.low.y = std::min(bounds.low.y, a.y);
        bounds.high.x = std::max(bounds.high.x, a.x);
        bounds.high.y = std::max(bounds.high.y, a.y);
    }
    return bounds;
}

double cameraClearance(Point point, const game_lab::Level& level,
                       const game_lab::Sector& sector, const game_lab::Polygon& polygon)
{
    double clearance = std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < polygon.vertices.size(); ++i) {
        clearance = std::min(clearance, segmentDistance(point,
            game_lab::worldVertex(sector, polygon.vertices[i]),
            game_lab::worldVertex(sector, polygon.vertices[(i + 1) % polygon.vertices.size()])));
    }
    // Include portals here: the initial camera must not lie on any wall segment.
    for (const auto& other : level.sectors) {
        for (const auto& wall : other.walls) {
            clearance = std::min(clearance, segmentDistance(point,
                game_lab::worldVertex(other, wall.vertexA), game_lab::worldVertex(other, wall.vertexB)));
        }
    }
    return clearance;
}

void sampleCameraPositions(const game_lab::Level& level, const game_lab::Sector& sector,
                           const game_lab::Polygon& polygon, const PolygonBounds& bounds,
                           double& bestClearance, Camera& best)
{
    // Preserve sample order and strict comparison so ties select the same position.
    // A concave polygon's centroid is not necessarily inside it.
    for (int y = 1; y < 20; ++y) {
        for (int x = 1; x < 20; ++x) {
            const Point point{bounds.low.x + (bounds.high.x - bounds.low.x) * x / 20.0,
                              bounds.low.y + (bounds.high.y - bounds.low.y) * y / 20.0};
            if (!collision::insidePolygon({point.x,point.y}, sector, polygon)) {
                continue;
            }
            const double clearance = cameraClearance(point, level, sector, polygon);
            if (clearance > bestClearance) {
                bestClearance = clearance;
                best = {point.x, point.y, 0};
            }
        }
    }
}

void faceLongestSightline(const game_lab::Level& level, Camera& camera)
{
    double longest = 0;
    for (int i = 0; i < 36; ++i) {
        const double angle = i * 2 * pi / 36;
        const auto hit = castWallRay(level, camera, std::cos(angle), std::sin(angle));
        if (std::isfinite(hit.distance) && hit.distance > longest) {
            longest = hit.distance;
            camera.angle = angle;
        }
    }
}

} // namespace

Camera initialCamera(const game_lab::Level& level)
{
    if (level.playerStart) {
        const auto& start=*level.playerStart;
        if (start.sector>=level.sectors.size())
            throw std::runtime_error("Invalid original player start sector");
        const auto& sector=level.sectors[start.sector];
        Camera camera{double(sector.gx)+start.x,double(sector.gy)+start.y,
                start.angle*(2*vortex::math::pi/720)};
        camera.sector=start.sector;
        return camera;
    }
    double bestClearance = 1.0;
    Camera best;
    for (const auto& sector : level.sectors) {
        for (const auto& polygon : sector.polygons) {
            if (polygon.vertices.size() < 3) {
                continue;
            }
            const auto bounds = polygonBounds(sector, polygon);
            if (std::abs(bounds.area2) < 1e-6) {
                continue;
            }
            sampleCameraPositions(level, sector, polygon, bounds, bestClearance, best);
        }
    }
    if (bestClearance > 1.0) {
        faceLongestSightline(level, best);
        return best;
    }
    throw std::runtime_error("No polygon interior safely clear of wall segments found");
}
