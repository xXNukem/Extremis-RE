#include "ray_scene.h"
#include "extremis/resources/sector_coordinates.h"

namespace extremis::render_detail {
namespace {
bool isStaticRayWall(const game_lab::Wall& wall)
{
    return (wall.type & 0xf000) != 0x1000;
}

template <typename Visitor>
void forEachStaticRayWall(const game_lab::Level& level, Visitor visit)
{
    // The grid and the prepared wall list must assign IDs in the same order.
    for (const auto& sector : level.sectors) {
        for (const auto& wall : sector.walls) {
            if (!isStaticRayWall(wall)) continue;
            const auto a = game_lab::worldVertex(sector, wall.vertexA);
            const auto b = game_lab::worldVertex(sector, wall.vertexB);
            visit(a,b,wall.type);
        }
    }
}

} // namespace

// Prepare camera-relative geometry each frame. Doors and the camera can move.
vortex::PreparedRayScene prepareRayScene(const game_lab::Level& level, const Camera& camera,
                                 const std::vector<doors::Door>* doors)
{
    vortex::PreparedRayScene scene;
    // Preserve the original sector/wall traversal order for exact tie-breaking.
    std::size_t count = 0;
    for (const auto& sector : level.sectors) count += sector.walls.size();
    scene.walls.reserve(count);
    const Point origin{camera.x,camera.y};
    forEachStaticRayWall(level,[&](Point a, Point b, std::uint16_t material) {
        scene.walls.push_back(vortex::prepareRaySurface(a,b,origin,material,1.0));
    });
    if (doors) {
        scene.dynamicSurfaces.reserve(doors->size() * 2);
        for (const auto& door : *doors) {
            for (const auto& closedFace : door.faces) {
                const auto face = doors::visibleFace(level, door, closedFace);
                if (!face) continue;
                const Point a{face->a.x, face->a.y};
                const Point b{face->b.x, face->b.y};
                const Point edge{b.x - a.x, b.y - a.y};
                // Doors use their own one-sided test; static walls stay two-sided.
                if (cross(edge, {camera.x - a.x, camera.y - a.y}) >= 0) continue;
                scene.dynamicSurfaces.push_back(vortex::prepareRaySurface(a,b,origin,
                    face->material,face->textureSpan));
            }
        }
    }
    return scene;
}

StaticRayGrid::StaticRayGrid(const game_lab::Level& level, bool force)
    : level_(&level), index([&](auto emit) {
        forEachStaticRayWall(level,[&](Point a, Point b, std::uint16_t) { emit(a,b); });
    },force) {}

bool StaticRayGrid::matches(const game_lab::Level& level) const { return level_ == &level; }
bool StaticRayGrid::valid() const { return index.valid(); }
std::size_t StaticRayGrid::bytes() const { return index.bytes() - sizeof(index) + sizeof(*this); }

double StaticRayGrid::backtrack(const Camera& camera) const { return index.backtrack({camera.x,camera.y}); }
vortex::RayHit StaticRayGrid::cast(const vortex::PreparedRayScene& scene, const Camera& camera, Point direction,
             double backtrack, unsigned* visitedCells, std::size_t* testedWalls)
{
    return index.cast(scene,{camera.x,camera.y},direction,backtrack,visitedCells,testedWalls);
}

} // namespace extremis::render_detail
