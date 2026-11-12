#pragma once
#include "player_collision.h"
#include "sprite_scene.h"

namespace collision {
unsigned objetExtent(unsigned bank, std::size_t image);
// DOS 0x10db7: candidate-point boxes, Y before X, optional enemy angle response (player leaves it null).
Point constrainObjets(const game_lab::Level& level, unsigned sector,
                      Point before, Point candidate, const sprites::Scene* scene = nullptr,
                      std::uint16_t* enemyAngle = nullptr);
}
