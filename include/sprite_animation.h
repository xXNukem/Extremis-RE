#pragma once

#include "sprite_scene.h"

namespace sprites {

// Original OBJET falling sprites. Real-time cadence is not calibrated.
class Animation {
public:
    static constexpr double provisionalStepSeconds = 0.1;
    void update(Scene& scene, double elapsedSeconds);

private:
    double remainder_ = 0;
};

} // namespace sprites
