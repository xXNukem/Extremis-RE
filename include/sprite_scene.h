#pragma once

#include "game_lab.h"
#include "sprite_bank.h"

#include <optional>

namespace sprites {

struct Instance {
    double x, y, z, height;
    std::size_t image;
    std::uint16_t type, param2;
};

struct Scene {
    Bank bank;
    std::vector<Instance> objects;
    std::size_t skipped = 0;
    unsigned objetBank = 0;
    bool animateImages = false;
};

std::optional<double> worldHeight(unsigned bank, std::size_t image);
void composeImages(Scene& scene, unsigned tick, bool scroll = true);
std::optional<std::size_t> imageIndex(std::uint16_t type, std::size_t count);
Scene makeScene(const game_lab::Level& level, Bank bank);
Scene loadScene(const game_lab::Level& level, const std::filesystem::path& directory);

} // namespace sprites
