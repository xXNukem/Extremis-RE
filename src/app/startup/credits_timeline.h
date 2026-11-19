#pragma once

#include "vortex/core/math/constants.h"
#include <algorithm>
#include <array>

namespace startup_credits {
inline constexpr float width=640.f, height=400.f;
inline constexpr double duration=29.4, firstEnd=7.5, secondEnd=16.8, titleStart=23.0, crtOffStart=28.05;
using vortex::math::pi;

struct Sweep { double at, travel; float damage; unsigned seed; };
inline constexpr std::array<Sweep,15> sweeps{{
    {.64,1.11,.08f, 0}, {2.42,1.08,.13f, 1}, {4.25,1.05,.16f, 2}, {6.02,.97,.21f, 3},
    {7.73,.98,.27f, 4}, {9.15,.92,.33f, 5}, {10.42,.88,.40f, 6},
    {11.66,.82,.49f, 7}, {12.81,.77,.56f, 8}, {13.89,.72,.65f, 9},
    {14.98,.72,.71f,10},
    {16.63,.89,.84f,11}, {17.58,.78,.92f,12},
    {18.46,.71,.98f,13}, {19.24,.66,1.00f,14}
}};

// Shared by the animation timeline and per-sample audio envelopes.
constexpr float fade(double t, double seconds)
{
    const double v=std::clamp(t/seconds,0.0,1.0);
    return float(v*v*(3.0-2.0*v));
}

}
