#pragma once

#include "texture_bank.h"

#include <array>

namespace wall_animation {

struct Overlay {
    std::size_t material, image;
    int x, y;
};

bool isAnimated(std::size_t material);
std::array<Overlay, 10> overlays(std::uint16_t tick);
void compose(wall_textures::Bank& materials, const wall_textures::Bank& images,
             std::uint16_t tick);

// Original sequences use one global 16-bit loop counter. Its wall-clock rate
// remains uncalibrated; 100 ms is explicitly provisional, not original timing.
class Clock {
public:
    static constexpr double provisionalTickSeconds = 0.1;
    bool advance(double elapsedSeconds);
    std::uint16_t tick() const;

private:
    double phase_ = 0.0;
};

} // namespace wall_animation
