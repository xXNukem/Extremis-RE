#include "player_collision.h"

#include <algorithm>
#include <cmath>
#include <optional>

namespace collision {
namespace {
Point add(Point a, Point b) { return {a.x + b.x, a.y + b.y}; }
Point subtract(Point a, Point b) { return {a.x - b.x, a.y - b.y}; }
Point scale(Point a, double factor) { return {a.x * factor, a.y * factor}; }
double dot(Point a, Point b) { return a.x * b.x + a.y * b.y; }
double length(Point a) { return std::hypot(a.x, a.y); }
double cross(Point a, Point b) { return a.x * b.y - a.y * b.x; }

Point closestPoint(Segment wall, Point point)
{
    const auto edge = subtract(wall.b, wall.a);
    const double extent2 = dot(edge, edge);
    const double u = extent2 > 0 ? std::clamp(dot(subtract(point, wall.a), edge) / extent2,
                                            0.0, 1.0) : 0;
    return add(wall.a, scale(edge, u));
}

struct Hit { double time; Point normal; };

void consider(std::optional<Hit>& best, double time, Point normal, Point motion)
{
    if (time < 0 || time > 1 || dot(motion, normal) >= -1e-10) return;
    if (!best || time < best->time) best = Hit{time, normal};
}

void sweepEndpoint(std::optional<Hit>& best, Point start, Point motion,
                   Point endpoint, double radius)
{
    const auto relative = subtract(start, endpoint);
    const long double a = dot(motion, motion), b = dot(relative, motion);
    const long double c = dot(relative, relative) - radius * radius;
    const long double discriminant = b * b - a * c;
    if (a == 0 || b >= 0 || discriminant < 0) return;
    // Stable entry root of a*t*t + 2*b*t + c = 0.
    const double time = double(c / (-b + std::sqrt(discriminant)));
    if (time < 0 || time > 1) return;
    const auto normal = add(relative, scale(motion, time));
    const double distance = length(normal);
    if (distance > 0) consider(best, time, scale(normal, 1 / distance), motion);
}

void sweepSides(std::optional<Hit>& best, Point start, Point motion,
                Segment wall, double radius)
{
    const auto edge = subtract(wall.b, wall.a);
    const double extent = length(edge);
    if (extent == 0) return;
    const auto tangent = scale(edge, 1 / extent);
    const Point normal{-tangent.y, tangent.x};
    const double velocity = dot(motion, normal);
    if (std::abs(velocity) < 1e-12) return;
    const double distance = dot(subtract(start, wall.a), normal);
    for (const double side : {-1.0, 1.0}) {
        const double time = (side * radius - distance) / velocity;
        if (time < 0 || time > 1) continue;
        const auto contact = add(start, scale(motion, time));
        const double along = dot(subtract(contact, wall.a), tangent);
        if (along >= 0 && along <= extent)
            consider(best, time, scale(normal, side), motion);
    }
}

void checkOverlap(std::optional<Hit>& best, Point start, Point motion,
                  Segment wall, double radius)
{
    const auto offset = subtract(start, closestPoint(wall, start));
    const double distance = length(offset);
    if (distance >= radius) return;
    // Invalid initial overlaps may escape; do not teleport the camera to another side.
    const auto normal = distance > 1e-12 ? scale(offset, 1 / distance)
                                       : scale(motion, -1 / length(motion));
    consider(best, 0, normal, motion);
}

std::optional<Hit> firstHit(const World& world, Point start, Point motion, double radius)
{
    std::optional<Hit> best;
    for (const auto& wall : world) {
        checkOverlap(best, start, motion, wall, radius);
        sweepSides(best, start, motion, wall, radius);
        sweepEndpoint(best, start, motion, wall.a, radius);
        sweepEndpoint(best, start, motion, wall.b, radius);
    }
    return best;
}

Point globalVertex(const game_lab::Sector& sector, std::uint16_t index)
{
    const auto& vertex = sector.vertices[index];
    return {double(sector.gx) + vertex.x, double(sector.gy) + vertex.y};
}
} // namespace

bool insidePolygon(Point point, const game_lab::Sector& sector, const game_lab::Polygon& polygon)
{
    if (polygon.vertices.size()<3) return false;
    bool inside=false;
    for (std::size_t i=0,j=polygon.vertices.size()-1;i<polygon.vertices.size();j=i++) {
        const auto a=globalVertex(sector,polygon.vertices[i]);
        const auto b=globalVertex(sector,polygon.vertices[j]);
        if (distanceToSegment({a,b},point)<1e-7) return true;
        if ((a.y>point.y)!=(b.y>point.y) &&
            point.x<(b.x-a.x)*(point.y-a.y)/(b.y-a.y)+a.x) inside=!inside;
    }
    return inside;
}
std::optional<Location> locate(const game_lab::Level& level, Point point, std::optional<unsigned> preferred)
{
    if (!std::isfinite(point.x) || !std::isfinite(point.y)) return {};
    if (preferred && *preferred<level.sectors.size()) {
        const auto& sector=level.sectors[*preferred];
        for (unsigned p=0;p<sector.polygons.size();++p)
            if (insidePolygon(point,sector,sector.polygons[p])) return Location{*preferred,p};
    }
    for (unsigned s=0;s<level.sectors.size();++s)
        for (unsigned p=0;p<level.sectors[s].polygons.size();++p)
            if (insidePolygon(point,level.sectors[s],level.sectors[s].polygons[p])) return Location{s,p};
    return {};
}

double distanceToSegment(Segment segment, Point point)
{
    return length(subtract(point, closestPoint(segment, point)));
}

double signedDistanceToLine(Segment segment, Point point)
{
    const auto edge = subtract(segment.b, segment.a);
    const double extent = length(edge);
    return extent > 0 ? cross(edge, subtract(point, segment.a)) / extent : 0;
}

bool overlaps(Segment segment, Point center, double radius)
{
    return distanceToSegment(segment, center) < radius;
}

std::optional<double> rayDistance(Segment segment, Point start, Point direction, double range)
{
    const auto edge = subtract(segment.b, segment.a), delta = subtract(segment.a, start);
    const double denominator = cross(direction, edge);
    if (std::abs(denominator) < 1e-12) {
        const double square = dot(direction, direction);
        if (square == 0 || std::abs(cross(delta, direction)) > 1e-9) return std::nullopt;
        const double a = dot(delta, direction) / square;
        const double b = dot(subtract(segment.b, start), direction) / square;
        if (std::max(a, b) < 0 || std::min(a, b) > range) return std::nullopt;
        return std::max(0.0, std::min(a, b));
    }
    const double time = cross(delta, edge) / denominator;
    const double along = cross(delta, direction) / denominator;
    if (time >= 0 && time <= range && along >= 0 && along <= 1) return time;
    return std::nullopt;
}

std::optional<double> rayDistance(const World& world, Point start, Point direction, double range)
{
    std::optional<double> nearest;
    for (const auto& segment : world) {
        const auto hit = rayDistance(segment, start, direction, nearest.value_or(range));
        if (hit) nearest = hit;
    }
    return nearest;
}

World staticWorld(const game_lab::Level& level)
{
    World world;
    for (const auto& sector : level.sectors) {
        for (const auto& wall : sector.walls) {
            if ((wall.type & 0xf000) == 0x1000) continue;
            world.push_back({globalVertex(sector, wall.vertexA),
                             globalVertex(sector, wall.vertexB)});
        }
    }
    return world;
}

Point move(const World& world, Point start, Point displacement, const Settings& settings)
{
    if (!std::isfinite(displacement.x) || !std::isfinite(displacement.y) ||
        !std::isfinite(settings.radius) || settings.radius <= 0 ||
        !std::isfinite(settings.skin) || settings.skin < 0) return start;
    for (unsigned i = 0; i < settings.maxContacts; ++i) {
        const double distance = length(displacement);
        if (distance < 1e-10) break;
        const auto hit = firstHit(world, start, displacement, settings.radius);
        if (!hit) return add(start, displacement);
        const double travel = std::max(0.0, hit->time - settings.skin / distance);
        start = add(start, scale(displacement, travel));
        displacement = scale(displacement, 1 - travel);
        const double inward = dot(displacement, hit->normal);
        if (inward < 0) displacement = subtract(displacement, scale(hit->normal, inward));
    }
    return start; // Never apply untested residual motion when the contact budget is exhausted.
}
} // namespace collision
