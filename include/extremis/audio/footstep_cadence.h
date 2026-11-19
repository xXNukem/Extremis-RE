#pragma once

#include <cstdint>

namespace audio {

// Distance adaptation of the original gait at its 18 FPS reference:
// phase +16 per 10 world units, threshold 144, post-request phase 28.
class FootstepCadence {
public:
    std::uint64_t advance(double effectiveDistance);

private:
    double distance_ = 0;
    bool first_ = true;
};

} // namespace audio
