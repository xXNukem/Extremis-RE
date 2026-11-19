#pragma once

namespace vortex {
struct Point {
    double x, y;
};

// A constexpr primitive so the raycast and collision loops can inline the determinant.
constexpr double cross(Point a, Point b)
{
    return a.x * b.y - a.y * b.x;
}
} // namespace vortex
