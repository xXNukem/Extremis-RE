#pragma once

#include "vortex/render/first_person/planes/plane_material.h"

namespace vortex::plane_material {
struct Sampling {
    double period;
    double seam;
    double texelsPerUnit;
    double fastPathLimit;
};

Sampling makeSampling(double repeatPeriod);
Texel coordinates(double worldX, double worldY, int width, int height, const Sampling& sampling);
} // namespace vortex::plane_material
