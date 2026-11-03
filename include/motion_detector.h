#pragma once
#include "camera.h"
#include <array>
#include <optional>
#include <vector>
namespace game_lab { struct Level; struct EnemyRecord; }
namespace detector {
struct Contact { int x=0,y=0; unsigned intensity=0; };
struct Reading {
    std::optional<unsigned> phase;
    std::array<Contact,10> contacts{};
    unsigned count=0;
};
Reading scan(const game_lab::Level& level, const std::vector<game_lab::EnemyRecord>& enemies,
             const Camera& camera, unsigned phase, bool enabled);
}
