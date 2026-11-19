#include "first_person_view.h"
#ifdef EXTREMIS_DEBUG_MENU
#include "vortex_debug_profiler.h" // VORTEX_DEBUG_PROFILER_V1
#endif
#include "wall_material.h"
#include "plane_renderer.h"
#include "sprite_renderer.h"
#include "player_collision.h"
#include "port_menu.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <new>
#include <optional>
#include <stdexcept>

namespace {
constexpr double pi = 3.14159265358979323846;
constexpr double wallHeight = 256.0;
constexpr double fov = Camera::horizontalFov;
constexpr double rayParallelTolerance = 1e-9;
constexpr double rayEndpointTolerance = 1e-9;
constexpr double rayTieTolerance = 1e-9;

bool isStaticRayWall(const game_lab::Wall& wall)
{
    return (wall.type & 0xf000) != 0x1000;
}

struct Point {
    double x, y;
};

struct PolygonBounds {
    Point low{1e30, 1e30};
    Point high{-1e30, -1e30};
    double area2 = 0;
};

struct CameraProjection {
    int width, height;
    double forwardX, forwardY;
    double plane, focal, eyeHeight;
};

struct ProjectedColumn {
    float top, bottom;
    double depth;
    double unclippedTop, unclippedBottom;
};

struct SegmentHit {
    double distance, position;
};

Point globalVertex(const game_lab::Sector& sector, std::uint16_t index)
{
    const auto& vertex = sector.vertices[index];
    return {double(sector.gx) + vertex.x, double(sector.gy) + vertex.y};
}

double cross(Point a, Point b)
{
    return a.x * b.y - a.y * b.x;
}

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
        const auto a = globalVertex(sector, polygon.vertices[i]);
        const auto b = globalVertex(sector, polygon.vertices[(i + 1) % polygon.vertices.size()]);
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
            globalVertex(sector, polygon.vertices[i]),
            globalVertex(sector, polygon.vertices[(i + 1) % polygon.vertices.size()])));
    }
    // Include portals here: the initial camera must not lie on any wall segment.
    for (const auto& other : level.sectors) {
        for (const auto& wall : other.walls) {
            clearance = std::min(clearance, segmentDistance(point,
                globalVertex(other, wall.vertexA), globalVertex(other, wall.vertexB)));
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

Point columnRay(const CameraProjection& projection, int column)
{
    const double offset = (2 * (column + 0.5) / projection.width - 1) * projection.plane;
    // Right = (sin(angle), -cos(angle)), matching the inverted map Y axis.
    // Do not normalize: forward dot ray = 1, so t is perpendicular depth.
    return {projection.forwardX + projection.forwardY * offset,
            projection.forwardY - projection.forwardX * offset};
}

ProjectedColumn projectWall(const CameraProjection& projection, double distance)
{
    const double depth = std::max(0.01, distance);
    const double unclippedTop = projection.height * 0.5 -
        (wallHeight - projection.eyeHeight) * projection.focal / depth;
    const double unclippedBottom = projection.height * 0.5 +
        projection.eyeHeight * projection.focal / depth;
    const float top = float(std::clamp(unclippedTop, 0.0, double(projection.height)));
    const float bottom = float(std::clamp(unclippedBottom, 0.0, double(projection.height)));
    return {top, bottom, depth, unclippedTop, unclippedBottom};
}

bool drawBackground(SDL_Renderer* renderer, int width, int height)
{
    const float split = float((height + 1) / 2);
    const SDL_FRect ceiling{0, 0, float(width), split};
    const SDL_FRect floor{0, split, float(width), float(height) - split};
    return SDL_SetRenderDrawColor(renderer, 20, 23, 32, 255) &&
           SDL_RenderFillRect(renderer, &ceiling) &&
           SDL_SetRenderDrawColor(renderer, 45, 39, 34, 255) &&
           SDL_RenderFillRect(renderer, &floor);
}

double distanceShade(double depth)
{
    return 0.2 + 0.8 / (1.0 + depth / 700.0);
}

SDL_Color wallBaseColor(std::uint16_t type)
{
    SDL_Color color{170, 170, 170, 255};
    switch (type & 0xf000) {
    case 0x2000: color = {255, 220, 60, 255}; break;
    case 0x3000: color = {255, 100, 190, 255}; break;
    }
    return color;
}

bool drawWallColumn(SDL_Renderer* renderer, int column, const ProjectedColumn& projected,
                    std::uint16_t type)
{
    const auto color = wallBaseColor(type);
    const double shade = distanceShade(projected.depth);
    return SDL_SetRenderDrawColor(renderer, Uint8(color.r * shade),
               Uint8(color.g * shade), Uint8(color.b * shade), 255) &&
           SDL_RenderLine(renderer, float(column), projected.top, float(column), projected.bottom);
}

std::uint32_t rgbaPixel(wall_textures::Rgb color, double shade = 1.0)
{
    return (std::uint32_t(color.r * shade) << 24) |
           (std::uint32_t(color.g * shade) << 16) |
           (std::uint32_t(color.b * shade) << 8) | 255;
}

void fillBackground(std::vector<std::uint32_t>& pixels, int width, int height)
{
    // The middle row of an odd-height view belongs to the upper color.
    const std::size_t upperPixels = std::size_t(height / 2 + height % 2) * width;
    SDL_memset4(pixels.data(), rgbaPixel({20, 23, 32}), upperPixels);
    SDL_memset4(pixels.data() + upperPixels, rgbaPixel({45, 39, 34}),
                pixels.size() - upperPixels);
}

void rasterizeWallColumn(std::vector<std::uint32_t>& pixels, int width, int height,
                            int column, const WallHit& hit, const ProjectedColumn& projected,
                            const wall_textures::Texture* texture,
                            std::vector<double>* depth = nullptr)
{
    // Frames span the first half of DAL's columns; other walls use the full image.
    const int columns = texture ? wallTextureColumnCount(hit.type, texture->width) : 0;
    const double textureExtent = columns * hit.textureSpan;
    const int lastColumn = texture ? std::max(0, int(std::ceil(textureExtent)) - 1) : 0;
    const int textureX = texture ? std::min(lastColumn,
        int(hit.segmentPosition * textureExtent)) : 0;
    const double projectedHeight = projected.unclippedBottom - projected.unclippedTop;
    const double shade = distanceShade(projected.depth);
    const int begin = int(std::ceil(std::clamp(projected.unclippedTop, 0.0,
                                              double(height)) - 0.5));
    const int end = int(std::ceil(std::clamp(projected.unclippedBottom, 0.0,
                                            double(height)) - 0.5));
    const auto base = wallBaseColor(hit.type);
    const auto flatPixel = rgbaPixel({base.r, base.g, base.b}, shade);
    // VORTEX_WALL_RASTER_FASTPATH_V1: a column reuses the same 74 source texels
    // many times at high resolution. Shade each texel once, not each output pixel.
    // The original path remains for short projected walls or other texture sizes.
    constexpr int cachedRows = 74;
    std::array<std::uint32_t, cachedRows> shadedTexels;
    const bool cachedShade = texture && texture->height == cachedRows && end - begin > cachedRows;
    if (cachedShade) {
        for (int texelY = 0; texelY < cachedRows; ++texelY) {
            const auto color = texture->pixels[std::size_t(texelY) * texture->width + textureX];
            shadedTexels[texelY] = rgbaPixel(color, shade);
        }
    }
    // This level has sealed floor/ceiling planes and full-height walls/doors.
    // Their nearest hit also hides billboard overhang above/below the wall's
    // screen rectangle (floor stains and ceiling fixtures use interior anchors).
    if (depth) {
        for (int y = 0; y < height; ++y)
            (*depth)[std::size_t(y) * width + column] = hit.distance;
    }
    if (cachedShade) {
        // Use the exact clipped-column sampler with a fixed texture height,
        // no per-pixel material branch, and a single stride increment.
        const auto* colors = shadedTexels.data();
        auto* output = pixels.data();
        auto offset = std::size_t(begin) * width + column;
        for (int y = begin; y < end; ++y, offset += width) {
            const double v = (y + 0.5 - projected.unclippedTop) / projectedHeight;
            const int texelY = int(v * cachedRows);
            const int row = texelY < 0 ? 0 : texelY >= cachedRows ? cachedRows - 1 : texelY;
            output[offset] = colors[row];
        }
        return;
    }
    for (int y = begin; y < end; ++y) {
        auto pixel = flatPixel;
        if (texture) {
            // Use the original projected extent: clipping crops rather than stretching.
            const double v = (y + 0.5 - projected.unclippedTop) / projectedHeight;
            const int textureY = std::clamp(int(v * texture->height), 0, texture->height - 1);
            const auto color = texture->pixels[std::size_t(textureY) * texture->width + textureX];
            pixel = rgbaPixel(color, shade);
        }
        pixels[std::size_t(y) * width + column] = pixel;
    }
}

bool validTexture(const wall_textures::Texture& texture)
{
    return texture.width > 0 && texture.height > 0 &&
           texture.pixels.size() == std::size_t(texture.width) * texture.height;
}

const wall_textures::Texture* selectWallTexture(const WallHit& hit,
                                               const wall_textures::Bank* materials,
                                               const wall_textures::Texture* overrideTexture)
{
    if (overrideTexture) {
        return overrideTexture;
    }
    if (!materials) return nullptr;
    const auto index = wallTextureIndex(hit.type, materials->textures.size());
    if (!index || !validTexture(materials->textures[*index])) {
        return nullptr; // Preserve a visible flat-color wall for unsupported materials.
    }
    return &materials->textures[*index];
}

// VORTEX_RAYCAST_PRECOMPUTE_V1: camera-relative ray geometry is prepared once
// per rendered frame. Do not cache across frames: levels and door phases change.
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
    std::vector<PreparedRaySurface> doors;
};

PreparedRaySurface prepareRaySurface(Point a, Point b, const Camera& camera,
                                     std::uint16_t material, double textureSpan)
{
    const Point edge{b.x - a.x, b.y - a.y};
    const Point delta{a.x - camera.x, a.y - camera.y};
    return {delta, edge, cross(delta, edge), material, textureSpan,
            cross(edge, {camera.x - a.x, camera.y - a.y}) < 0};
}

PreparedRayScene prepareRayScene(const game_lab::Level& level, const Camera& camera,
                                 const std::vector<doors::Door>* doors)
{
    PreparedRayScene scene;
    // Preserve the original sector/wall traversal order for exact tie-breaking.
    std::size_t count = 0;
    for (const auto& sector : level.sectors) count += sector.walls.size();
    scene.walls.reserve(count);
    for (const auto& sector : level.sectors) {
        for (const auto& wall : sector.walls) {
            if (!isStaticRayWall(wall)) continue;
            const auto a = globalVertex(sector, wall.vertexA);
            const auto b = globalVertex(sector, wall.vertexB);
            scene.walls.push_back(prepareRaySurface(a, b, camera, wall.type, 1.0));
        }
    }
    if (doors) {
        scene.doors.reserve(doors->size() * 2);
        for (const auto& door : *doors) {
            for (const auto& closedFace : door.faces) {
                const auto face = doors::visibleFace(level, door, closedFace);
                if (!face) continue;
                const Point a{face->a.x, face->a.y};
                const Point b{face->b.x, face->b.y};
                const Point edge{b.x - a.x, b.y - a.y};
                // Doors use their own one-sided test; static walls stay two-sided.
                if (cross(edge, {camera.x - a.x, camera.y - a.y}) >= 0) continue;
                scene.doors.push_back(prepareRaySurface(a, b, camera,
                                                      face->material, face->textureSpan));
            }
        }
    }
    return scene;
}

std::optional<SegmentHit> intersectPreparedRay(const PreparedRaySurface& surface,
                                                Point direction)
{
    // Same arithmetic and tolerances as intersectSegment. Skip the U division
    // unless the ray actually reaches the segment in front of the camera.
    const double denominator = cross(direction, surface.edge);
    if (std::abs(denominator) < rayParallelTolerance) return std::nullopt;
    const double t = surface.numerator / denominator;
    if (!(t > 1e-6)) return std::nullopt;
    const double u = cross(surface.delta, direction) / denominator;
    if (u >= -rayEndpointTolerance && u <= 1 + rayEndpointTolerance)
        return SegmentHit{t, std::clamp(u, 0.0, 1.0)};
    return std::nullopt;
}

void considerWallHit(WallHit& hit, bool& hitFacesCamera, const PreparedRaySurface& wall,
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

WallHit castPreparedDoors(const PreparedRayScene& scene, Point direction, WallHit hit)
{
    // A door replaces the closest static hit only if strictly nearer.
    for (const auto& door : scene.doors) {
        const auto intersection = intersectPreparedRay(door, direction);
        if (intersection && intersection->distance < hit.distance)
            hit = {intersection->distance, door.material, intersection->position,
                   door.textureSpan};
    }
    return hit;
}

WallHit castPreparedSceneRay(const PreparedRayScene& scene, Point direction)
{
    WallHit hit{std::numeric_limits<double>::infinity(), 0};
    bool hitFacesCamera = false;
    for (const auto& wall : scene.walls) {
        const auto intersection = intersectPreparedRay(wall, direction);
        if (intersection) considerWallHit(hit, hitFacesCamera, wall, *intersection);
    }
    return castPreparedDoors(scene, direction, hit);
}

} // namespace

class StaticRayGrid {
public:
    // Small scenes favor the contiguous prepared scan. The forced path is used
    // by the diagnostic to verify and measure the grid even in those scenes.
    static constexpr std::size_t minimumWalls = 64;
    explicit StaticRayGrid(const game_lab::Level& level, bool force = false)
        : level_(&level)
    {
        struct Bounds { Point low, high; };
        std::vector<Bounds> bounds;
        Point low{INFINITY, INFINITY}, high{-INFINITY, -INFINITY};
        double maxEdge = 0;
        for (const auto& sector : level.sectors) {
            for (const auto& wall : sector.walls) {
                if (!isStaticRayWall(wall)) continue;
                const auto a = globalVertex(sector, wall.vertexA);
                const auto b = globalVertex(sector, wall.vertexB);
                Bounds box{{std::min(a.x, b.x) - padding, std::min(a.y, b.y) - padding},
                           {std::max(a.x, b.x) + padding, std::max(a.y, b.y) + padding}};
                bounds.push_back(box);
                low = {std::min(low.x, box.low.x), std::min(low.y, box.low.y)};
                high = {std::max(high.x, box.high.x), std::max(high.y, box.high.y)};
                maxEdge = std::max({maxEdge, std::abs(b.x - a.x), std::abs(b.y - a.y)});
            }
        }
        if (bounds.empty() || (!force && bounds.size() < minimumWalls)) return;
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

    bool matches(const game_lab::Level& level) const { return level_ == &level; }
    bool valid() const { return !cells_.empty(); }
    std::size_t bytes() const
    {
        std::size_t result = sizeof(*this) + cells_.capacity() * sizeof(cells_.front()) +
                             (masks_.capacity() + hitMasks_.capacity()) * sizeof(masks_.front()) +
                             intersections_.capacity() * sizeof(intersections_.front());
        for (const auto& masks : cells_) result += masks.capacity() * sizeof(CellMask);
        return result;
    }

    double backtrack(const Camera& camera) const
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

    WallHit cast(const PreparedRayScene& scene, const Camera& camera, Point direction,
                 double backtrack, unsigned* visitedCells = nullptr, std::size_t* testedWalls = nullptr)
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
        const auto count = collect({camera.x, camera.y}, direction, backtrack,
                                   walls, testedWalls);
        if (visitedCells) *visitedCells += count;
        WallHit hit{std::numeric_limits<double>::infinity(), 0};
        bool hitFacesCamera = false;
        // Replay only accepted intersections in the original wall order.
        for (std::size_t block = 0; block < hitMasks_.size(); ++block) {
            for (auto bits = hitMasks_[block]; bits; bits &= bits - 1) {
                const auto id = block * 64 + firstBit(bits);
                considerWallHit(hit, hitFacesCamera, walls[id], intersections_[id]);
            }
        }
        return castPreparedDoors(scene, direction, hit);
    }

private:
    static constexpr double cellSize = 512;
    // Larger than endpoint extension (<= 131070 * 1e-9) and the transverse
    // roundoff of the accepted U and of the traversal at map coordinate scale.
    static constexpr double padding = 0.01;
    struct CellMask { std::uint64_t bits; unsigned block; };

    static unsigned firstBit(std::uint64_t bits)
    {
#if defined(__GNUC__) || defined(__clang__)
        return static_cast<unsigned>(__builtin_ctzll(bits));
#else
        unsigned index = 0;
        for (; (bits & 1) == 0; bits >>= 1) ++index;
        return index;
#endif
    }

    static int cell(double position, double origin)
    {
        return static_cast<int>((position - origin) / cellSize);
    }

    static bool clipAxis(double start, double speed, double low, double high,
                         double& entry, double& exit)
    {
        if (speed == 0) return start >= low && start <= high;
        const double a = (low - start) / speed, b = (high - start) / speed;
        entry = std::max(entry, std::min(a, b));
        exit = std::min(exit, std::max(a, b));
        return entry <= exit;
    }

    unsigned collect(Point camera, Point direction, double backtrack,
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

    const game_lab::Level* level_;
    Point origin_{};
    int width_ = 0, height_ = 0;
    std::size_t wallCount_ = 0;
    double longitudinalErrorScale_ = 0, maxEdge_ = 0;
    std::vector<std::vector<CellMask>> cells_;
    std::vector<std::uint64_t> masks_, hitMasks_;
    std::vector<SegmentHit> intersections_;
};

namespace {

template <typename DrawColumn>
bool renderVisibleColumns(const game_lab::Level& level, const Camera& camera,
                          int width, int height, const std::vector<doors::Door>* doors,
                          DrawColumn drawColumn, int referenceWidth = 0,
                          StaticRayGrid* grid = nullptr)
{
    // Preserve the original vertical field of view. Extra width adds rays on
    // BOTH sides instead of stretching the same 320 columns horizontally.
    const double reference = referenceWidth > 0 ? referenceWidth : width;
    const double plane = std::tan(fov * 0.5) * double(width) / reference;
    const CameraProjection projection{width, height, std::cos(camera.angle),
                                      std::sin(camera.angle), plane, width / (2 * plane), camera.viewHeight()};
#ifdef EXTREMIS_DEBUG_MENU
    const auto prepareRayStart = SDL_GetTicksNS();
#endif
    const auto preparedRayScene = prepareRayScene(level, camera, doors);
    const double backtrack = grid ? grid->backtrack(camera) : 0;
#ifdef EXTREMIS_DEBUG_MENU
    vortex_debug_profiler::add(vortex_debug_profiler::Rays,
                               SDL_GetTicksNS() - prepareRayStart);
#endif
    for (int column = 0; column < width; ++column) {
        const auto direction = columnRay(projection, column);
#ifdef EXTREMIS_DEBUG_MENU
        const auto rayBegin=SDL_GetTicksNS();
#endif
        const auto hit = grid ? grid->cast(preparedRayScene, camera, direction, backtrack)
                              : castPreparedSceneRay(preparedRayScene, direction);
#ifdef EXTREMIS_DEBUG_MENU
        vortex_debug_profiler::add(vortex_debug_profiler::Rays,SDL_GetTicksNS()-rayBegin);
#endif
        if (!std::isfinite(hit.distance)) {
            continue;
        }
        if (!drawColumn(column, hit, projectWall(projection, hit.distance))) {
            return false;
        }
    }
    return true;
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
                start.angle*(2*3.14159265358979323846/720)};
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
            const auto a = globalVertex(sector, wall.vertexA);
            const auto b = globalVertex(sector, wall.vertexB);
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

namespace {

bool drawFlatView(SDL_Renderer* renderer, const game_lab::Level& level, const Camera& camera,
                  const std::vector<doors::Door>* doors, StaticRayGrid* grid)
{
    int width = 0, height = 0;
    if (!SDL_GetRenderOutputSize(renderer, &width, &height)) {
        return false;
    }
    if (width <= 0 || height <= 0) {
        return true;
    }
    if (!drawBackground(renderer, width, height)) {
        return false;
    }
    return renderVisibleColumns(level, camera, width, height, doors,
        [&](int column, const WallHit& hit, const ProjectedColumn& projected) {
            return drawWallColumn(renderer, column, projected, hit.type);
        }, 0, grid);
}

} // namespace

bool drawFirstPerson(SDL_Renderer* renderer, const game_lab::Level& level,
                     const Camera& camera, const std::vector<doors::Door>* doors)
{
    return drawFlatView(renderer, level, camera, doors, nullptr);
}

FirstPersonRenderer::FirstPersonRenderer(SDL_Renderer* renderer) : renderer_(renderer) {}
FirstPersonRenderer::~FirstPersonRenderer() = default;

void FirstPersonRenderer::setRaycastLevel(const game_lab::Level& level)
{
    rayGrid_.reset();
    try {
        auto grid = std::make_unique<StaticRayGrid>(level);
        if (grid->valid()) rayGrid_ = std::move(grid);
    } catch (const std::bad_alloc&) {
        // The renderer can still use the prepared scan without an index.
    }
}

void FirstPersonRenderer::TextureDeleter::operator()(SDL_Texture* texture) const
{
    SDL_DestroyTexture(texture);
}

bool FirstPersonRenderer::resizeFrame(int width, int height, bool withHud)
{
    const bool nativeView=withHud && portVideoNativeMax;
    if (width == width_ && height == height_ && withHud == withHud_ && nativeView==nativeComposed_) {
        return true;
    }
    // RGBA8888 specifies channel bit positions, independent of host byte order.
    std::unique_ptr<SDL_Texture, TextureDeleter> texture(SDL_CreateTexture(
        renderer_, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
        width, withHud ? (nativeView && height>hud::viewHeight
            ? int(std::lround(double(height)*hud::height/hud::viewHeight))
            : hud::height * (height / hud::viewHeight)) : height));
    if (!texture || !SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_NONE) ||
        !SDL_SetTextureScaleMode(texture.get(), SDL_SCALEMODE_NEAREST)) {
        return false;
    }
    frameReady_=false; showingMap_=false;
    pixels_.resize(std::size_t(width) * height);
    composed_.resize(withHud ? hud::width * hud::height : 0);
    wideComposed_.resize(nativeView && height>hud::viewHeight
        ? std::size_t(width)*int(std::lround(double(height)*hud::height/hud::viewHeight))
        : (withHud && height>hud::viewHeight
        ? std::size_t(width)*(height+(hud::height-hud::viewHeight)*(height/hud::viewHeight))
        : (withHud && width>hud::width ? std::size_t(width)*hud::height : 0)));
    uploadTexture_ = std::move(texture);
    width_ = width;
    height_ = height;
    withHud_ = withHud;
    nativeComposed_ = nativeView;
    if (withHud) {
        int drawableWidth=0,drawableHeight=0;
        SDL_GetRenderOutputSize(renderer_,&drawableWidth,&drawableHeight);
        SDL_Log("VORTEX VIDEO: salida SDL=%dx%d; selector=%dx%d; mundo 3D=%dx%d; HUD DOS=320x200",
                drawableWidth,drawableHeight,
                portVideoRequestedPixelWidth,portVideoRequestedPixelHeight,width_,height_);
    }
    return true;
}

bool FirstPersonRenderer::presentFrame(bool withHud, int outputWidth, int outputHeight,
                                       const hud::Renderer* hudRenderer)
{
#ifdef EXTREMIS_DEBUG_MENU
    vortex_debug_profiler::Scope compositionTimer(vortex_debug_profiler::Composition);
#endif

    // VORTEX_INTERNAL_RESOLUTION_V1
    // Only the 3D scene uses the larger framebuffer. The DOS UI keeps its art.
    // VORTEX_VIDEO_RESOLUTION_V3: arbitrary physical MAX mode,
    // including half/in-between scales without supersampling above the display.
    if (withHud && nativeComposed_ && height_>hud::viewHeight) {
        if(!hudRenderer) return SDL_SetError("Missing HUD for native resolution");
        const double scale=double(height_)/hud::viewHeight;
        const double baseWidth=double(width_)/scale;
        const double inset=(baseWidth-hud::width)*.5;
        const int fullHeight=int(std::lround(double(height_)*hud::height/hud::viewHeight));
        if(wideComposed_.size()!=std::size_t(width_)*fullHeight)
            return SDL_SetError("Invalid native video framebuffer");
        std::copy(pixels_.begin(),pixels_.end(),wideComposed_.begin());
        constexpr int cornerCounts[10]={60,48,40,32,24,20,16,12,8,4};
        for(int y=0;y<hud::viewHeight;++y) {
            const int corner=y<10?cornerCounts[y]:(y>=140?cornerCounts[9-(y-140)]:0);
            const int startY=std::clamp(int(std::lround(y*scale)),0,height_);
            const int endY=std::clamp(int(std::lround((y+1)*scale)),0,height_);
            const int sourceY=std::clamp(int((y+.5)*scale),0,height_-1);
            for(int x=0;x<hud::width;++x) {
                const auto overlay=composed_[std::size_t(y)*hud::width+x];
                const int sourceX=std::clamp(int((inset+x+.5)*scale),0,width_-1);
                if(overlay==pixels_[std::size_t(sourceY)*width_+sourceX]) continue;
                double placedX=inset+x;
                if(baseWidth>hud::width) {
                    if(x<corner) placedX=x;
                    else if(x>=hud::width-corner) placedX=baseWidth-hud::width+x;
                }
                const int startX=std::clamp(int(std::lround(placedX*scale)),0,width_);
                const int endX=std::clamp(int(std::lround((placedX+1)*scale)),0,width_);
                for(int yy=startY;yy<endY;++yy) {
                    auto* row=wideComposed_.data()+std::size_t(yy)*width_;
                    std::fill(row+startX,row+std::max(startX,endX),overlay);
                }
            }
        }
        // Row-correct extension of the original 320x50 instrument panel.
        int previousPanelY=-1;
        for(int y=height_;y<fullHeight;++y) {
            const int panelY=std::clamp(int((y-height_)/scale),0,hud::height-hud::viewHeight-1);
            auto* row=wideComposed_.data()+std::size_t(y)*width_;
            // Consecutive physical rows sampling the same HUD row are identical.
            if(panelY==previousPanelY) {
                std::copy_n(row-width_,width_,row);
                continue;
            }
            previousPanelY=panelY;
            for(int x=0;x<width_;++x) {
                const double logicalX=x/scale-inset;
                if(logicalX<0) row[x]=hudRenderer->panelBackgroundEdge(panelY,false);
                else if(logicalX>=hud::width) row[x]=hudRenderer->panelBackgroundEdge(panelY,true);
                else row[x]=composed_[std::size_t(panelY+hud::viewHeight)*hud::width+int(logicalX)];
            }
        }
        if(!SDL_UpdateTexture(uploadTexture_.get(),nullptr,wideComposed_.data(),
                              width_*int(sizeof(std::uint32_t)))) return false;
        const float ratio=(4.f/3.f)*(float(width_)/(float(hud::width)*float(scale)));
        const float w=std::min(float(outputWidth),float(outputHeight)*ratio);
        const float h=w/ratio;
        const SDL_FRect destination{(outputWidth-w)*.5f,(outputHeight-h)*.5f,w,h};
        return SDL_RenderTexture(renderer_,uploadTexture_.get(),nullptr,&destination);
    }
    if (withHud && height_ > hud::viewHeight) {
        if (!hudRenderer) return SDL_SetError("Missing original HUD for high-resolution view");
        const int scale = height_ / hud::viewHeight;
        if (scale < 2 || width_ % scale) return SDL_SetError("Invalid internal rendering scale");
        const int baseWidth = width_ / scale;
        if (baseWidth < hud::width || wideComposed_.size() !=
            std::size_t(width_)*(height_+(hud::height-hud::viewHeight)*scale))
            return SDL_SetError("Invalid high-resolution composition dimensions");

        const int inset = (baseWidth - hud::width)/2;
        std::copy(pixels_.begin(), pixels_.end(), wideComposed_.begin());

        // DOS overlay is captured independently of any 3D postprocess, so
        // fisheye never distorts HUD, crosshair, weapon or interaction text.
        constexpr int cornerCounts[10]={60,48,40,32,24,20,16,12,8,4};
        for (int y=0; y<hud::viewHeight; ++y) {
            const int corner=(y<10) ? cornerCounts[y]
                : (y>=140 ? cornerCounts[9-(y-140)] : 0);
            for (int x=0; x<hud::width; ++x) {
                const auto overlay = composed_[std::size_t(y)*hud::width+x];
                const int sourceX = ((inset+x)*scale+scale/2);
                const int sourceY = (y*scale+scale/2);
                if (overlay==pixels_[std::size_t(sourceY)*width_+sourceX]) continue;
                int placedX = inset+x;
                if (baseWidth>hud::width) {
                    if (x<corner) placedX=x;
                    else if (x>=hud::width-corner) placedX=baseWidth-hud::width+x;
                }
                for (int dy=0;dy<scale;++dy) {
                    auto* row=wideComposed_.data()+std::size_t(y*scale+dy)*width_;
                    std::fill_n(row+placedX*scale,scale,overlay);
                }
            }
        }

        // The 50-line original HUD panel is centred, not stretched sideways.
        for (int panelY=0;panelY<hud::height-hud::viewHeight;++panelY) {
            for (int x=0;x<baseWidth;++x) {
                const auto color=x<inset
                    ? hudRenderer->panelBackgroundEdge(panelY,false)
                    : (x>=inset+hud::width
                        ? hudRenderer->panelBackgroundEdge(panelY,true)
                        : composed_[std::size_t(panelY+hud::viewHeight)*hud::width+(x-inset)]);
                for (int dy=0;dy<scale;++dy) {
                    auto* row=wideComposed_.data()+
                              std::size_t(height_+panelY*scale+dy)*width_;
                    std::fill_n(row+x*scale,scale,color);
                }
            }
        }
        if (!SDL_UpdateTexture(uploadTexture_.get(),nullptr,wideComposed_.data(),
                               width_*int(sizeof(std::uint32_t)))) return false;
        const float ratio=(4.f/3.f)*(float(baseWidth)/float(hud::width));
        const float w=std::min(float(outputWidth),float(outputHeight)*ratio);
        const float h=w/ratio;
        const SDL_FRect destination{(outputWidth-w)*.5f,(outputHeight-h)*.5f,w,h};
        return SDL_RenderTexture(renderer_,uploadTexture_.get(),nullptr,&destination);
    }

    const std::vector<std::uint32_t>* frame = withHud ? &composed_ : &pixels_;
    if (withHud && width_>hud::width) {
        if (!hudRenderer) return SDL_SetError("Missing original HUD for widescreen panel");
        // The world is the full width of the screen. Only the DOS artwork
        // remains at its original 320-pixel width in the centre.
        const int inset=(width_-hud::width)/2;
        constexpr int cornerCounts[10]={60,48,40,32,24,20,16,12,8,4};
        for (int y=0;y<hud::viewHeight;++y) {
            auto* target=wideComposed_.data()+std::size_t(y)*width_;
            const auto* world=pixels_.data()+std::size_t(y)*width_;
            std::copy_n(world,width_,target);
            for (int x=0;x<hud::width;++x) {
                const auto overlay=composed_[std::size_t(y)*hud::width+x];
                if (overlay==world[inset+x]) continue;
                // Keep the original DOS helmet corner shapes at the edges
                // of the *extended* viewport rather than in the old 4:3 area.
                const int corner=(y<10)?cornerCounts[y]:
                                 (y>=140?cornerCounts[9-(y-140)]:0);
                if (x<corner) target[x]=overlay;
                else if (x>=hud::width-corner) target[width_-hud::width+x]=overlay;
                else target[inset+x]=overlay;
            }
        }
        // Extend the actual CASQUE[1] background in each row, without
        // stretching or repeating the original instruments or helmet geometry.
        // The DOS 320x50 panel remains pixel-for-pixel unchanged in the middle.
        // The right-side count also handles odd widescreen widths correctly.
        for (int y=hud::viewHeight;y<hud::height;++y) {
            const int panelRow=y-hud::viewHeight;
            auto* destination=wideComposed_.data()+std::size_t(y)*width_;
            const auto* original=composed_.data()+std::size_t(y)*hud::width;
            std::fill_n(destination,inset,hudRenderer->panelBackgroundEdge(panelRow,false));
            std::copy_n(original,hud::width,destination+inset);
            std::fill_n(destination+inset+hud::width,width_-inset-hud::width,
                        hudRenderer->panelBackgroundEdge(panelRow,true));
        }
        frame=&wideComposed_;
    }
    if (!SDL_UpdateTexture(uploadTexture_.get(), nullptr, frame->data(),
                           width_ * int(sizeof(std::uint32_t)))) return false;
    if (!withHud) return SDL_RenderTexture(renderer_, uploadTexture_.get(), nullptr, nullptr);
    // 320x200 was displayed on a 4:3 CRT (non-square VGA pixels).
    // Wider game canvases keep the same pixel aspect, without pillarboxing.
    const float ratio=(4.f/3.f)*(float(width_)/float(hud::width));
    const float w=std::min(float(outputWidth),float(outputHeight)*ratio);
    const float h=w/ratio;
    const SDL_FRect destination{(outputWidth-w)*.5f,(outputHeight-h)*.5f,w,h};
    return SDL_RenderTexture(renderer_, uploadTexture_.get(), nullptr, &destination);
}
bool FirstPersonRenderer::drawFrame(const std::vector<std::uint32_t>& pixels)
{
    if(pixels.size()!=64000) return SDL_SetError("Invalid presentation framebuffer");
    if(!resizeFrame(320,150,true)) return false;
    composed_=pixels;
    int width,height;
    return SDL_GetRenderOutputSize(renderer_,&width,&height) && presentFrame(true,width,height);
}

bool FirstPersonRenderer::draw(const game_lab::Level& level, const Camera& camera,
                               const wall_textures::Texture* provisionalTexture,
                               const wall_textures::Bank* materials,
                               const std::vector<doors::Door>* doors,
                               const wall_textures::PlaneBank* planes,
                               const sprites::Scene* spriteScene,
                               const hud::Renderer* hudRenderer, const hud::State& hudState,
                               const interaction::Renderer* interactionRenderer,
                               const interaction::State* interactionState, const sprites::Scene* projectileScene, const sprites::Scene* enemyScene, const std::vector<sprites::Instance>* enemyEffects,
                               const original_map::Renderer* map, const original_map::State* mapState)
{
    auto* grid = rayGrid_ && rayGrid_->matches(level) ? rayGrid_.get() : nullptr;
    if (!provisionalTexture && !materials && !planes && !spriteScene && !projectileScene && !hudRenderer) {
        return drawFlatView(renderer_, level, camera, doors, grid);
    }
    if (provisionalTexture && !validTexture(*provisionalTexture)) {
        return SDL_SetError("Invalid wall texture dimensions or pixel count");
    }
    int width = 0, height = 0;
    if (!SDL_GetRenderOutputSize(renderer_, &width, &height)) {
        return false;
    }
    if (width <= 0 || height <= 0) {
        return true;
    }
    const int outputWidth = width, outputHeight = height;
    if (hudRenderer) {
        const double monitorAspect=double(outputWidth)/outputHeight;
        const double aspect=portVideoAspectChoice==0 ? 4.0/3.0 :
                            portVideoAspectChoice==1 ? 16.0/9.0 :
                            portVideoAspectChoice==2 ? 21.0/9.0 : monitorAspect;
        // Width is measured in the original DOS game's non-square pixels;
        // its native 320x200 image corresponds to a 4:3 physical display.
        width=std::clamp(int(std::lround(hud::width*aspect/(4.0/3.0))),
                         hud::width,hud::width*3);
        height=hud::viewHeight;
    }
    // Keep 1x fallback, enable 2x at 1080p, up to 3x at 1440p/4K.
    // VORTEX_VIDEO_RESOLUTION_V2: internal 3D follows the selected
    // physical display mode, not merely an unchanging Retina backbuffer.
    // DOS HUD remains 320x200 and the whole view preserves its aspect.
    // The 6x safety limit prevents unbounded software rendering at 5K/8K.
    const int baseViewWidth=width;
    const bool nativeMax=hudRenderer && portVideoNativeMax;
    const int renderScale=hudRenderer?std::max(1,portVideoRenderScale):1;
    if(nativeMax && portVideoNativeWidth>0 && portVideoNativeHeight>0) {
        const double virtualAspect=(4.0/3.0)*baseViewWidth/hud::width;
        const double physicalWidth=std::min(double(portVideoNativeWidth),
                                            double(portVideoNativeHeight)*virtualAspect);
        const double physicalHeight=physicalWidth/virtualAspect;
        height=std::max(hud::viewHeight,int(std::lround(physicalHeight*hud::viewHeight/240.0)));
        width=std::max(baseViewWidth,int(std::lround(double(baseViewWidth)*height/hud::viewHeight)));
        width=std::min(width,std::max(baseViewWidth,int(std::floor(physicalWidth))));
    } else {
        width*=renderScale;
        height*=renderScale;
    }
    const int projectionReferenceWidth=hudRenderer
        ? int(std::lround(double(width)*hud::width/baseViewWidth)) : width;
    if (!resizeFrame(width, height, hudRenderer != nullptr)) {
        return false;
    }
    if (hudRenderer && map && mapState && frameReady_) {
        if (!showingMap_) mapBackground_=composed_;
        showingMap_=true;
        composed_=mapBackground_;
        if (!map->draw(composed_,*mapState)) return SDL_SetError("Invalid original map framebuffer");
        return presentFrame(true,outputWidth,outputHeight,hudRenderer);
    }
    showingMap_=false;
    fillBackground(pixels_, width, height);
    const bool requiresDepth = spriteScene || projectileScene || enemyScene;
    if (requiresDepth) {
        depth_.resize(pixels_.size());
        // First row is a scratch array of nearest-wall depths until raster ends.
        std::fill_n(depth_.begin(), width, std::numeric_limits<double>::infinity());
    }
    const double focal = projectionReferenceWidth / (2 * std::tan(fov * 0.5));
    // Resolve and rasterize the nearest wall before sampling the planes.
    // Untouched pixels will receive precisely their historical floor/ceiling texel.
    if (planes) coveredWallRows_.assign(std::size_t(width), {height, 0});
    const bool wallsDrawn = renderVisibleColumns(level, camera, width, height, doors,
        [&](int column, const WallHit& hit, const ProjectedColumn& projected) {
            const auto* texture = selectWallTexture(hit, materials, provisionalTexture);
#ifdef EXTREMIS_DEBUG_MENU
            const auto wallBegin=SDL_GetTicksNS();
#endif
            rasterizeWallColumn(pixels_, width, height, column, hit, projected, texture,
                                nullptr);
            if (requiresDepth) depth_[column] = hit.distance;
#ifdef EXTREMIS_DEBUG_MENU
            vortex_debug_profiler::add(vortex_debug_profiler::Walls,SDL_GetTicksNS()-wallBegin);
#endif
            if (planes) {
                // Match rasterizeWallColumn's exact half-pixel raster bounds.
                // No approximation: pixels covered by walls are left intact.
                const int first=int(std::ceil(std::clamp(projected.unclippedTop,0.0,double(height))-.5));
                const int last=int(std::ceil(std::clamp(projected.unclippedBottom,0.0,double(height))-.5));
                coveredWallRows_[column]={first,last};
            }
            return true;
        }, hudRenderer ? projectionReferenceWidth : 0, grid);
    if (!wallsDrawn) return false;
    if (requiresDepth) {
#ifdef EXTREMIS_DEBUG_MENU
        const auto wallDepthStart = SDL_GetTicksNS();
#endif
        // VORTEX_WALL_RASTER_FASTPATH_V1: contiguous row-major depth writes.
        // Even columns without a wall retain +infinity, exactly as before.
        for (int y = 1; y < height; ++y)
            std::copy_n(depth_.data(), width, depth_.data() + std::size_t(y) * width);
#ifdef EXTREMIS_DEBUG_MENU
        vortex_debug_profiler::add(vortex_debug_profiler::Walls,
                                   SDL_GetTicksNS() - wallDepthStart);
#endif
    }
    if (planes) {
#ifdef EXTREMIS_DEBUG_MENU
        vortex_debug_profiler::Scope timer(vortex_debug_profiler::Planes);
#endif
        // VORTEX_HIDDEN_PLANE_CULLING_V1: only shade uncovered pixels.
        planes::render(pixels_,camera,{width,height,focal,camera.viewHeight(),wallHeight},
                       *planes,&coveredWallRows_);
    }
#ifdef EXTREMIS_DEBUG_MENU
    const auto spritesBegin=SDL_GetTicksNS();
#endif
    // The buffer starts with infinity and receives only finite wall and sprite depths.
    if (requiresDepth)
        wallDepthColumns_.assign(depth_.data(), depth_.data() + width);
    const sprites::Projection spriteProjection{width, height, focal, camera.viewHeight(),
                                               requiresDepth ? wallDepthColumns_.data() : nullptr};
    constexpr auto spriteDepthInput = sprites::DepthInput::NoNaN;
    if (spriteScene) sprites::render(pixels_, depth_, *spriteScene, camera, spriteProjection, spriteDepthInput);
    if (enemyScene) sprites::render(pixels_, depth_, *enemyScene, camera, spriteProjection, spriteDepthInput);
    if (enemyScene && enemyEffects) sprites::render(pixels_, depth_, enemyScene->bank, *enemyEffects, camera,
                                                   spriteProjection, spriteDepthInput);
    if (projectileScene) sprites::render(pixels_, depth_, *projectileScene, camera, spriteProjection, spriteDepthInput);
    // Apply barrel distortion only to the world, before drawing any DOS
    // weapon, reticle, text, corners, indicators or modal interfaces.
    if (portVideoFisheyeEnabled && !fisheye_.apply(pixels_,width,height))
        return SDL_SetError("Invalid fisheye world framebuffer");
#ifdef EXTREMIS_DEBUG_MENU
    vortex_debug_profiler::add(vortex_debug_profiler::Sprites,SDL_GetTicksNS()-spritesBegin);
    const auto hudBegin=SDL_GetTicksNS();
#endif
    if (hudRenderer) {
        if (nativeMax && height>hud::viewHeight) {
            const double scale=double(height)/hud::viewHeight;
            const double inset=(double(width)/scale-hud::width)*.5;
            for(int y=0;y<hud::viewHeight;++y) {
                const int sy=std::clamp(int((y+.5)*scale),0,height-1);
                for(int x=0;x<hud::width;++x) {
                    const int sx=std::clamp(int((inset+x+.5)*scale),0,width-1);
                    composed_[std::size_t(y)*hud::width+x]=pixels_[std::size_t(sy)*width+sx];
                }
            }
        } else if (height>hud::viewHeight) {
            // The original HUD receives a native 320x150 snapshot while the
            // high-res, possibly fisheye-warped 3D remains untouched.
            const int inset=(width/renderScale-hud::width)/2;
            for(int y=0;y<hud::viewHeight;++y)
                for(int x=0;x<hud::width;++x)
                    composed_[std::size_t(y)*hud::width+x]=
                        pixels_[std::size_t(y*renderScale+renderScale/2)*width+
                                (inset+x)*renderScale+renderScale/2];
        } else if (width==hud::width) {
            std::copy(pixels_.begin(),pixels_.end(),composed_.begin());
        } else {
            // The DOS interface still receives precisely 320x200 pixels.
            // Its view area is a centred crop of the expanded world.
            const int inset=(width-hud::width)/2;
            for(int y=0;y<hud::viewHeight;++y)
                std::copy_n(pixels_.data()+std::size_t(y)*width+inset,hud::width,
                            composed_.data()+std::size_t(y)*hud::width);
        }
if (!hudRenderer->draw(composed_,hudState)) return SDL_SetError("Invalid HUD framebuffer");
        if (interactionRenderer && interactionState && !interactionRenderer->draw(composed_,*interactionState))
            return SDL_SetError("Invalid original interaction framebuffer");
        if (map && mapState) {
            mapBackground_=composed_; showingMap_=true;
            if (!map->draw(composed_,*mapState)) return SDL_SetError("Invalid original map framebuffer");
        }
    }
#ifdef EXTREMIS_DEBUG_MENU
    vortex_debug_profiler::add(vortex_debug_profiler::Hud,SDL_GetTicksNS()-hudBegin);
#endif
    frameReady_=true;
    return presentFrame(hudRenderer != nullptr,outputWidth,outputHeight,hudRenderer);
}
