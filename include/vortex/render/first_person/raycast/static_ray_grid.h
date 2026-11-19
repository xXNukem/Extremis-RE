#pragma once

#include "vortex/render/first_person/raycast/raycast_geometry.h"

#include <cstddef>

namespace vortex {
class StaticRayGrid {
public:
    // Small scenes favor the contiguous prepared scan. The forced path is used
    // by the diagnostic to verify and measure the grid even in those scenes.
    static constexpr std::size_t minimumWalls = 64;
    // Enumerate calls its visitor with (start, end) in the same order in
    // which the caller fills PreparedRayScene::walls. The visitor is valid only
    // during enumeration. Rebuild on geometry changes.
    // The indexed profile accepts coordinates in [-262144, 262144] and
    // segment component lengths up to 131070. Other scenes use the scan.
    template <typename EnumerateSegments>
    explicit StaticRayGrid(EnumerateSegments enumerate, bool force = false);

    bool valid() const;
    std::size_t bytes() const;

    // Reuse this value only for casts from the same origin and through this index.
    double backtrack(const Point& camera) const;

    RayHit cast(const PreparedRayScene& scene, const Point& camera, Point direction, double backtrack,
                unsigned* visitedCells = nullptr, std::size_t* testedWalls = nullptr);

private:
    struct SegmentBounds;
    struct SegmentVisitor {
        SegmentBounds* bounds;
        void operator()(Point a, Point b) const;
    };
    using Enumerate = void (*)(void*, SegmentVisitor);

    void build(void* enumerator, Enumerate enumerate, bool force);

    static constexpr double cellSize = 512;
    // Larger than endpoint extension (<= 131070 * 1e-9) and the transverse
    // roundoff of the accepted U and of the traversal at map coordinate scale.
    static constexpr double padding = 0.01;
    struct CellMask { std::uint64_t bits; unsigned block; };

    static unsigned firstBit(std::uint64_t bits);

    static int cell(double position, double origin);

    static bool clipAxis(double start, double speed, double low, double high,
                         double& entry, double& exit);

    unsigned collect(Point camera, Point direction, double backtrack,
                     const PreparedRaySurface* walls, std::size_t* testedWalls);

    Point origin_{};
    int width_ = 0, height_ = 0;
    std::size_t wallCount_ = 0;
    double longitudinalErrorScale_ = 0, maxEdge_ = 0;
    std::vector<std::vector<CellMask>> cells_;
    std::vector<std::uint64_t> masks_, hitMasks_;
    std::vector<SegmentHit> intersections_;
};

} // namespace vortex

#include "vortex/render/first_person/raycast/static_ray_grid.tpp"
