#include "sprite_animation.h"

#include <cmath>

namespace sprites {
namespace {
bool falls(unsigned bank, std::size_t image)
{
    switch (bank) {
    case 1: return image == 14;
    case 2: return image == 3;
    case 3: return image == 5;
    case 4: return image == 11 || image == 15;
    case 6: return image == 2 || image == 5;
    case 7: return image == 2 || image == 23;
    case 8: return image == 12 || image == 14;
    default: return false;
    }
}
} // namespace

void Animation::update(Scene& scene, double elapsedSeconds)
{
    if (!std::isfinite(elapsedSeconds) || elapsedSeconds <= 0) return;
    // 64 original steps repeat the vertical phase (64 * 20 is divisible by 256).
    constexpr double cycleSeconds = 64 * provisionalStepSeconds;
    const double accumulated = remainder_ + std::fmod(elapsedSeconds, cycleSeconds);
    unsigned steps = unsigned(std::floor(accumulated / provisionalStepSeconds + 1e-9));
    remainder_ = accumulated - steps * provisionalStepSeconds;
    if (remainder_ < 0) remainder_ = 0;
    if (steps == 0 && elapsedSeconds >= cycleSeconds) steps = 64;
    if (steps == 0) return;
    for (auto& object : scene.objects) {
        if (!falls(scene.objetBank, object.image)) continue;
        // Preserve initial z=256 until the first update; subsequent values wrap
        // into [0,256), including z=0, as in SUB 20 / JNS / ADD 256.
        double z = std::fmod(object.z - 20.0 * steps, 256.0);
        if (z < 0) z += 256;
        object.z = z;
    }
}

} // namespace sprites
