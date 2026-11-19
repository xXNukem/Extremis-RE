#pragma once

#include "vortex/render/first_person/camera_pose.h"
#include "vortex/render/first_person/planes/plane_material.h"
#include "vortex/render/first_person/postprocess/fisheye_effect.h"
#include "vortex/render/first_person/raycast/static_ray_grid.h"
#include "vortex/render/first_person/sprites/sprite_types.h"
#include "vortex/render/first_person/walls/wall_raster.h"

namespace vortex::first_person {

struct ColumnProjection {
    int width, height;
    double forwardX, forwardY;
    double plane, focal, eyeHeight;
};

// Kept inline for both the renderer and clients that query individual columns.
constexpr Point columnRay(const ColumnProjection& projection, int column)
{
    const double offset = (2 * (column + 0.5) / projection.width - 1) * projection.plane;
    return {projection.forwardX + projection.forwardY * offset,
            projection.forwardY - projection.forwardX * offset};
}

enum class Stage { Rays, Walls, Planes, Sprites };
struct Timing {
    void* context = nullptr;
    std::uint64_t (*now)() = nullptr;
    void (*sample)(void*, Stage, std::uint64_t) = nullptr;
};

// Prepared surfaces use the camera origin. The optional grid indexes scene.walls
// in the same order. Forward is unit length and rays are left unnormalized.
struct View {
    CameraPose camera;
    ColumnProjection projection;
    double wallHeight;
    // Kept explicit for callers with independently rounded plane/sprite focal lengths.
    double planeAndSpriteFocal;
    const PreparedRayScene& scene;
    StaticRayGrid* grid = nullptr;
};

using ColumnVisitor = bool (*)(void*, int, const RayHit&, const ProjectedColumn&);
bool visitWallColumns(const View& view, ColumnVisitor visitor, void* context,
                      Timing timing = {});

struct WallSample {
    const Texture* texture = nullptr;
    int textureX = 0;
    Rgb color{};
};
using WallMaterial = WallSample (*)(const void*, const RayHit&);

struct SpriteLayer {
    const sprites::Bank* bank;
    const std::vector<sprites::Instance>* objects;
};

struct SceneStyle {
    WallMaterial wallMaterial = nullptr;
    const void* wallContext = nullptr;
    std::uint32_t ceilingColor = 0, floorColor = 0;
    const PlaneBank* planes = nullptr;
    double planeRepeat = 0;
    const SpriteLayer* layers = nullptr;
    std::size_t layerCount = 0;
    FisheyeEffect* fisheye = nullptr;
    // Optional observation before and after each active sprite layer.
    void (*observeLayer)(void*, std::size_t, bool after, const std::vector<std::uint32_t>&) = nullptr;
    void* layerContext = nullptr;
};

struct RenderBuffers {
    // Depth is populated when sprite layers are supplied. Coverage is populated
    // when planes are supplied. Both use world framebuffer coordinates.
    std::vector<double> depth;
    std::vector<std::pair<int, int>> wallCoverage;
    std::vector<double> wallDepthColumns;
};

// Pixels must contain exactly width * height entries.
void fillBackground(std::vector<std::uint32_t>& pixels, int width, int height,
                    std::uint32_t ceiling, std::uint32_t floor);

// The caller owns the framebuffer and reusable scratch buffers. Fisheye, when
// enabled, warps color only after the ordered sprite layers have been drawn.
bool render(const View& view, const SceneStyle& style, std::vector<std::uint32_t>& pixels,
            RenderBuffers& buffers, Timing timing = {});

} // namespace vortex::first_person
