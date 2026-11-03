#pragma once
#include "player_collision.h"

namespace enemies {
// Routes through original sector links and polygon connection waypoints.
std::optional<collision::Point> waypoint(const game_lab::Level& level,
    collision::Location from, collision::Point start, collision::Location target,
    collision::Point goal);
}
