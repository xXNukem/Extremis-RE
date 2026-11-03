#pragma once

#include "sprite_scene.h"

namespace sprites {

// Level-1 drops: EXE 0x11d0f..0x11d66. Real-time cadence is not calibrated.
class Animation {
public:
    static constexpr double provisionalStepSeconds = 0.1;
    void update(Scene& scene, double elapsedSeconds);

private:
    double remainder_ = 0;
};

} // namespace sprites
