#include "vortex/render/first_person/scene_renderer.h"

#include "vortex/render/first_person/planes/plane_renderer.h"
#include "vortex/render/first_person/sprites/sprite_renderer.h"

#include <algorithm>
#include <cmath>
#include <limits>

#ifdef __APPLE__
#include <cstring>
#endif

namespace vortex::first_person {
namespace {
// Select timing once per view, keeping clock checks out of the untimed column loop.
template <bool Enabled>
class Sample {
public:
    Sample(const Timing& timing, Stage stage)
        : timing_(timing), stage_(stage), start_(Enabled ? timing.now() : 0) {}
    ~Sample()
    {
        if constexpr (Enabled) timing_.sample(timing_.context, stage_, timing_.now() - start_);
    }

private:
    const Timing& timing_;
    Stage stage_;
    std::uint64_t start_;
};

template <bool Timed, typename Visitor>
bool walkColumns(const View& view, Visitor visit, const Timing& timing)
{
    const auto& p = view.projection;
    const Point origin{view.camera.x, view.camera.y};
    double backtrack = 0;
    {
        const Sample<Timed> sample(timing, Stage::Rays);
        if (view.grid) backtrack = view.grid->backtrack(origin);
    }

    for (int column = 0; column < p.width; ++column) {
        const auto direction = columnRay(p, column);
        const auto hit = [&] {
            const Sample<Timed> sample(timing, Stage::Rays);
            return view.grid ? view.grid->cast(view.scene, origin, direction, backtrack)
                             : castPreparedSceneRay(view.scene, direction);
        }();
        if (!std::isfinite(hit.distance)) continue;
        const auto projected = projectWall(p.height, p.focal, p.eyeHeight,
                                           view.wallHeight, hit.distance);
        if (!visit(column, hit, projected)) return false;
    }
    return true;
}

template <bool Timed>
bool renderScene(const View& view, const SceneStyle& style, std::vector<std::uint32_t>& pixels,
                 RenderBuffers& buffers, const Timing& timing)
{
    const auto& p = view.projection;
    if (p.width <= 0 || p.height <= 0) return false;

    pixels.resize(std::size_t(p.width) * p.height);
    fillBackground(pixels, p.width, p.height, style.ceilingColor, style.floorColor);
    const bool requiresDepth = style.layerCount != 0;
    auto& depth = buffers.depth;
    if (requiresDepth) {
        depth.resize(pixels.size());
        // The first row holds the nearest wall distance until all walls are drawn.
        std::fill_n(depth.begin(), p.width, std::numeric_limits<double>::infinity());
    }
    if (style.planes) buffers.wallCoverage.assign(std::size_t(p.width), {p.height, 0});

    walkColumns<Timed>(view, [&](int column, const RayHit& hit, const ProjectedColumn& projected) {
        const auto material = style.wallMaterial
            ? style.wallMaterial(style.wallContext, hit) : WallSample{};
        const Sample<Timed> sample(timing, Stage::Walls);
        const auto rows = rasterizeWallColumn(pixels, p.width, p.height, column, hit.distance,
                                             projected, material.texture, material.textureX,
                                             material.color);
        if (requiresDepth) depth[column] = hit.distance;
        if (style.planes) buffers.wallCoverage[column] = rows;
        return true;
    }, timing);

    if (requiresDepth) {
        const Sample<Timed> sample(timing, Stage::Walls);
        for (int y = 1; y < p.height; ++y)
            std::copy_n(depth.data(), p.width, depth.data() + std::size_t(y) * p.width);
    }

    if (style.planes) {
        const Sample<Timed> sample(timing, Stage::Planes);
        planes::render(pixels, view.camera,
                       {p.width, p.height, view.planeAndSpriteFocal, p.eyeHeight, view.wallHeight},
                       *style.planes, style.planeRepeat, &buffers.wallCoverage);
    }

    const Sample<Timed> sample(timing, Stage::Sprites);
    if (requiresDepth) {
        buffers.wallDepthColumns.assign(depth.data(), depth.data() + p.width);
        const sprites::Projection projection{p.width, p.height, view.planeAndSpriteFocal,
                                             p.eyeHeight, buffers.wallDepthColumns.data()};
        for (std::size_t i = 0; i < style.layerCount; ++i) {
            const auto& layer = style.layers[i];
            if (!layer.bank || !layer.objects) continue;
            if (style.observeLayer) style.observeLayer(style.layerContext, i, false, pixels);
            sprites::render(pixels, depth, *layer.bank, *layer.objects, view.camera, projection,
                            sprites::DepthInput::NoNaN);
            if (style.observeLayer) style.observeLayer(style.layerContext, i, true, pixels);
        }
    }

    return !style.fisheye || style.fisheye->apply(pixels, p.width, p.height);
}

} // namespace

bool visitWallColumns(const View& view, ColumnVisitor visitor, void* context, Timing timing)
{
    const auto visit = [&](int column, const RayHit& hit, const ProjectedColumn& projected) {
        return visitor(context, column, hit, projected);
    };
    if (timing.now && timing.sample) return walkColumns<true>(view, visit, timing);
    return walkColumns<false>(view, visit, timing);
}

void fillBackground(std::vector<std::uint32_t>& pixels, int width, int height,
                    std::uint32_t ceiling, std::uint32_t floor)
{
    const auto upperPixels = std::size_t(height / 2 + height % 2) * width;
#ifdef __APPLE__
    // Preserve the bulk fill used by SDL on Darwin, including in Debug builds.
    if (pixels.empty()) return;
    memset_pattern4(pixels.data(), &ceiling, upperPixels * sizeof(ceiling));
    memset_pattern4(pixels.data() + upperPixels, &floor,
                    (pixels.size() - upperPixels) * sizeof(floor));
#else
    std::fill_n(pixels.begin(), upperPixels, ceiling);
    std::fill(pixels.begin() + upperPixels, pixels.end(), floor);
#endif
}

bool render(const View& view, const SceneStyle& style, std::vector<std::uint32_t>& pixels,
            RenderBuffers& buffers, Timing timing)
{
    if (timing.now && timing.sample) return renderScene<true>(view, style, pixels, buffers, timing);
    return renderScene<false>(view, style, pixels, buffers, timing);
}

} // namespace vortex::first_person
