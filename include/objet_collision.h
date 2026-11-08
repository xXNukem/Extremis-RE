#pragma once
#include "player_collision.h"
#include "sprite_scene.h"

namespace collision {
unsigned objetExtent(unsigned bank, std::size_t image);
// Player mode of DOS 0x10db7: candidate-point boxes, Y before X, no angle change.
Point constrainObjets(const game_lab::Level& level, unsigned sector,
                      Point before, Point candidate, const sprites::Scene* scene = nullptr);
}
