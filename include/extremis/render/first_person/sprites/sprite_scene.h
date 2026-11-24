#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>
#include "extremis/resources/game_lab.h"
#include "extremis/resources/sprite_bank.h"

#include <optional>

namespace sprites {

using Instance = vortex::sprites::Instance;

struct ObjectPlacement {
    std::size_t sourceSector, sourceEntity;
    std::uint16_t type, param2;
};

struct Scene {
    Bank bank;
    std::vector<Instance> objects;
    std::size_t skipped = 0;
    unsigned objetBank = 0;
    bool animateImages = false;
    // OBJET scenes keep one original placement per object in matching order.
    // Animations update the render object while its placement stays unchanged.
    std::vector<ObjectPlacement> placements;
};

std::optional<double> worldHeight(unsigned bank, std::size_t image);
void composeImages(Scene& scene, unsigned tick, bool scroll = true);
std::optional<std::size_t> imageIndex(std::uint16_t type, std::size_t count);
Scene makeScene(const game_lab::Level& level, Bank bank);
Scene loadScene(const game_lab::Level& level, const std::filesystem::path& directory);

} // namespace sprites
