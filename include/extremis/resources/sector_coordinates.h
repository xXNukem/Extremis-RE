#pragma once

#include "extremis/resources/game_lab.h"
#include "vortex/core/geometry/point.h"

#include <cstdint>

namespace game_lab {
// Convert a vertex after the caller has established that the index is valid.
// The linear reference queries use this operation twice per candidate wall.
inline vortex::Point worldVertex(const Sector& sector, std::uint16_t index)
{
    const auto& vertex = sector.vertices[index];
    return {double(sector.gx) + vertex.x, double(sector.gy) + vertex.y};
}
} // namespace game_lab
