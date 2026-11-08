#pragma once

#include "sprite_scene.h"

namespace sprites {

// Shared decorative animation phase. 18 Hz is a port adaptation, not DOS timing.
class DecorativeAnimation {
public:
    static constexpr double stepSeconds = 1.0 / 18.0;
    void update(Scene& scene, double elapsedSeconds);
    void step(Scene& scene);
    std::uint16_t tick() const { return tick_; }

private:
    double remainder_ = 0;
    std::uint16_t tick_ = 0;
};

} // namespace sprites
