#include "vortex/render/first_person/raycast/static_ray_grid.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace vortex {

PreparedRaySurface prepareRaySurface(Point a, Point b, Point camera,
                                     std::uint16_t material, double textureSpan)
{
    const Point edge{b.x - a.x, b.y - a.y};
    const Point delta{a.x - camera.x, a.y - camera.y};
    return {delta, edge, cross(delta, edge), material, textureSpan,
            cross(edge, {camera.x - a.x, camera.y - a.y}) < 0};
}

std::optional<SegmentHit> intersectPreparedRay(const PreparedRaySurface& surface,
                                               Point direction)
{
    // Skip the U division unless the ray reaches the segment in front of the camera.
    const double denominator = cross(direction, surface.edge);
    if (std::abs(denominator) < rayParallelTolerance) return std::nullopt;
    const double t = surface.numerator / denominator;
    if (!(t > 1e-6)) return std::nullopt;
    const double u = cross(surface.delta, direction) / denominator;
    if (u >= -rayEndpointTolerance && u <= 1 + rayEndpointTolerance)
        return SegmentHit{t, std::clamp(u, 0.0, 1.0)};
    return std::nullopt;
}

void considerWallHit(RayHit& hit, bool& hitFacesCamera, const PreparedRaySurface& wall,
                     const SegmentHit& intersection)
{
    const bool coincident = std::isfinite(hit.distance) &&
        std::abs(intersection.distance - hit.distance) <=
            rayTieTolerance * std::max(1.0, std::abs(hit.distance));
    if (coincident ? (wall.facesCamera && !hitFacesCamera)
                   : intersection.distance < hit.distance) {
        hit = {intersection.distance, wall.material, intersection.position};
        hitFacesCamera = wall.facesCamera;
    }
}

RayHit castDynamicSurfaces(const PreparedRayScene& scene, Point direction, RayHit hit)
{
    // A dynamic surface replaces the current hit only if strictly nearer.
    for (const auto& surface : scene.dynamicSurfaces) {
        const auto intersection = intersectPreparedRay(surface, direction);
        if (intersection && intersection->distance < hit.distance)
            hit = {intersection->distance, surface.material, intersection->position,
                   surface.textureSpan};
    }
    return hit;
}

RayHit castPreparedSceneRay(const PreparedRayScene& scene, Point direction)
{
    RayHit hit{std::numeric_limits<double>::infinity(), 0};
    bool hitFacesCamera = false;
    for (const auto& wall : scene.walls) {
        const auto intersection = intersectPreparedRay(wall, direction);
        if (intersection) considerWallHit(hit, hitFacesCamera, wall, *intersection);
    }
    return castDynamicSurfaces(scene, direction, hit);
}

struct StaticRayGrid::SegmentBounds {
    struct Box { Point low, high; };

    std::vector<Box> bounds;
    Point low{INFINITY, INFINITY}, high{-INFINITY, -INFINITY};
    double maxEdge = 0;
    bool supported = true;
};

void StaticRayGrid::SegmentVisitor::operator()(Point a, Point b) const
{
    auto& [boxes, low, high, maxEdge, supported] = *bounds;
    const auto supportedPoint = [](Point point) {
        return std::abs(point.x) <= 262144 && std::abs(point.y) <= 262144;
    };
    if (!supported || !supportedPoint(a) || !supportedPoint(b)) {
        supported = false;
        return;
    }
    const SegmentBounds::Box box{{std::min(a.x, b.x) - padding, std::min(a.y, b.y) - padding},
                                 {std::max(a.x, b.x) + padding, std::max(a.y, b.y) + padding}};
    boxes.push_back(box);
    low = {std::min(low.x, box.low.x), std::min(low.y, box.low.y)};
    high = {std::max(high.x, box.high.x), std::max(high.y, box.high.y)};
    maxEdge = std::max({maxEdge, std::abs(b.x - a.x), std::abs(b.y - a.y)});
}

void StaticRayGrid::build(void* enumerator, Enumerate enumerate, bool force)
{
    SegmentBounds segments;
    enumerate(enumerator, SegmentVisitor{&segments});
    const auto& [bounds, low, high, maxEdge, supported] = segments;

    if (!supported || maxEdge > 131070 || bounds.empty() ||
        (!force && bounds.size() < minimumWalls)) return;
    origin_ = {std::floor(low.x / cellSize) * cellSize,
               std::floor(low.y / cellSize) * cellSize};
    width_ = cell(high.x, origin_.x) + 1;
    height_ = cell(high.y, origin_.y) + 1;
    const auto cellCount = std::size_t(width_) * height_;
    if (cellCount > 16384 || bounds.size() > 65536) return;
    cells_.resize(cellCount);
    masks_.resize((bounds.size() + 63) / 64);
    wallCount_ = bounds.size();
    hitMasks_.resize(masks_.size());
    intersections_.resize(wallCount_);

    // Closed, padded AABBs include the cells on either side of an edge or
    // corner, including for rays which run along a grid line.
    for (std::size_t id = 0; id < bounds.size(); ++id) {
        const auto& box = bounds[id];
        const auto block = static_cast<unsigned>(id / 64);
        const auto bit = std::uint64_t{1} << (id % 64);
        for (int y = cell(box.low.y, origin_.y); y <= cell(box.high.y, origin_.y); ++y) {
            for (int x = cell(box.low.x, origin_.x); x <= cell(box.high.x, origin_.x); ++x) {
                auto& masks = cells_[std::size_t(y) * width_ + x];
                if (masks.empty() || masks.back().block != block)
                    masks.push_back({bit, block});
                else
                    masks.back().bits |= bit;
            }
        }
    }
    longitudinalErrorScale_ = 32 * std::numeric_limits<double>::epsilon() *
                              maxEdge / rayParallelTolerance;
    maxEdge_ = maxEdge;
}

bool StaticRayGrid::valid() const { return !cells_.empty(); }

std::size_t StaticRayGrid::bytes() const
{
    std::size_t result = sizeof(*this) + cells_.capacity() * sizeof(cells_.front()) +
                         (masks_.capacity() + hitMasks_.capacity()) * sizeof(masks_.front()) +
                         intersections_.capacity() * sizeof(intersections_.front());
    for (const auto& masks : cells_) result += masks.capacity() * sizeof(CellMask);
    return result;
}

double StaticRayGrid::backtrack(const Point& camera) const
{
    // d*t = delta + u*edge in exact arithmetic. Near the denominator
    // cutoff, rounding in the determinants can put the reconstructed
    // segment point slightly behind the origin of an accepted positive t.
    // Bound that error using the largest camera-to-vertex offset. Direction
    // magnitude cancels when converting the error to ray parameters.
    const double offset = std::max({std::abs(camera.x - origin_.x),
        std::abs(camera.x - (origin_.x + width_ * cellSize)),
        std::abs(camera.y - origin_.y),
        std::abs(camera.y - (origin_.y + height_ * cellSize))});
    return longitudinalErrorScale_ * (offset + 2 * maxEdge_ + 1) + padding;
}

RayHit StaticRayGrid::cast(const PreparedRayScene& scene, const Point& camera, Point direction, double backtrack,
                          unsigned* visitedCells, std::size_t* testedWalls)
{
    const double magnitude = std::max(std::abs(direction.x), std::abs(direction.y));
    if (!valid() || wallCount_ != scene.walls.size() ||
        !(std::abs(camera.x) <= 262144 && std::abs(camera.y) <= 262144 &&
          std::isfinite(direction.x) && std::isfinite(direction.y) &&
          magnitude >= 0.25 && magnitude <= 4)) {
        if (testedWalls) *testedWalls += scene.walls.size();
        return castPreparedSceneRay(scene, direction);
    }

    std::fill(masks_.begin(), masks_.end(), 0);
    std::fill(hitMasks_.begin(), hitMasks_.end(), 0);
    const auto* walls = scene.walls.data();
    const auto count = collect(camera, direction, backtrack, walls, testedWalls);
    if (visitedCells) *visitedCells += count;
    RayHit hit{std::numeric_limits<double>::infinity(), 0};
    bool hitFacesCamera = false;
    // Replay only accepted intersections in the original wall order.
    for (std::size_t block = 0; block < hitMasks_.size(); ++block) {
        for (auto bits = hitMasks_[block]; bits; bits &= bits - 1) {
            const auto id = block * 64 + firstBit(bits);
            considerWallHit(hit, hitFacesCamera, walls[id], intersections_[id]);
        }
    }
    return castDynamicSurfaces(scene, direction, hit);
}

    unsigned StaticRayGrid::firstBit(std::uint64_t bits)
    {
#if defined(__GNUC__) || defined(__clang__)
        return static_cast<unsigned>(__builtin_ctzll(bits));
#else
        unsigned index = 0;
        for (; (bits & 1) == 0; bits >>= 1) ++index;
        return index;
#endif
    }

int StaticRayGrid::cell(double position, double origin)
{
    return static_cast<int>((position - origin) / cellSize);
}

bool StaticRayGrid::clipAxis(double start, double speed, double low, double high,
                     double& entry, double& exit)
{
    if (speed == 0) return start >= low && start <= high;
    const double a = (low - start) / speed, b = (high - start) / speed;
    entry = std::max(entry, std::min(a, b));
    exit = std::min(exit, std::max(a, b));
    return entry <= exit;
}

unsigned StaticRayGrid::collect(Point camera, Point direction, double backtrack,
                 const PreparedRaySurface* walls, std::size_t* testedWalls)
{
    double entry = -backtrack, exit = INFINITY;
    if (!clipAxis(camera.x, direction.x, origin_.x, origin_.x + width_ * cellSize, entry, exit) ||
        !clipAxis(camera.y, direction.y, origin_.y, origin_.y + height_ * cellSize, entry, exit))
        return 0;

    const Point start{camera.x + entry * direction.x, camera.y + entry * direction.y};
    int x = std::clamp(cell(start.x, origin_.x), 0, width_ - 1);
    int y = std::clamp(cell(start.y, origin_.y), 0, height_ - 1);
    const int stepX = direction.x < 0 ? -1 : 1, stepY = direction.y < 0 ? -1 : 1;
    const double deltaX = direction.x == 0 ? INFINITY : cellSize / std::abs(direction.x);
    const double deltaY = direction.y == 0 ? INFINITY : cellSize / std::abs(direction.y);
    double nextX = direction.x == 0 ? INFINITY :
        (origin_.x + (x + (stepX > 0)) * cellSize - start.x) / direction.x;
    double nextY = direction.y == 0 ? INFINITY :
        (origin_.y + (y + (stepY > 0)) * cellSize - start.y) / direction.y;

    unsigned count = 0;
    double nearest = INFINITY, stop = INFINITY;
    // DDA times are relative to entry. Allow for determinant error in t
    // (the same bound used by backtrack) and for grid crossing roundoff.
    const double margin = 2 * backtrack - entry;
    const double tieSteps = 4 * rayTieTolerance * wallCount_;
    auto* seen = masks_.data();
    auto* accepted = hitMasks_.data();
    auto* intersections = intersections_.data();
    do {
        ++count;
        for (const auto& mask : cells_[std::size_t(y) * width_ + x]) {
            auto& visited = seen[mask.block];
            auto pending = mask.bits & ~visited;
            visited |= mask.bits;
            for (; pending; pending &= pending - 1) {
                const auto bit = firstBit(pending);
                const auto id = mask.block * 64 + bit;
                const auto& wall = walls[id];
                if (testedWalls) ++*testedWalls;
                // Preserve the in-loop checks: sharing the optional intersection
                // path adds spills in unoptimized builds.
                const double denominator = cross(direction, wall.edge);
                if (std::abs(denominator) < rayParallelTolerance) continue;
                const double t = wall.numerator / denominator;
                if (!(t > 1e-6)) continue;
                const double u = cross(wall.delta, direction) / denominator;
                if (!(u >= -rayEndpointTolerance && u <= 1 + rayEndpointTolerance)) continue;
                intersections[id] = {t, std::clamp(u, 0.0, 1.0)};
                accepted[mask.block] |= std::uint64_t{1} << bit;
                if (t < nearest) {
                    nearest = t;
                    // A single tie band is insufficient: an earlier, farther
                    // hit can affect a chain of source-order comparisons.
                    // With N walls and one witness at t, one of the N open
                    // bands above t must be empty. Each band is wider than
                    // the tie tolerance across it, so hits beyond that gap
                    // cannot affect selection on the witness side.
                    stop = t + tieSteps * std::max(1.0, t) + margin;
                }
            }
        }
        if (nextX > stop && nextY > stop) break;
        const bool crossX = nextX <= nextY, crossY = nextY <= nextX;
        if (crossX) { x += stepX; nextX += deltaX; }
        if (crossY) { y += stepY; nextY += deltaY; }
    } while (x >= 0 && x < width_ && y >= 0 && y < height_);
    return count;
}

} // namespace vortex
