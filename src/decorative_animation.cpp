#include "decorative_animation.h"

#include <cmath>

namespace sprites {

void DecorativeAnimation::step(Scene& scene)
{
    // Increment once per fixed simulation step; zero after wrap is also even.
    tick_ = static_cast<std::uint16_t>(tick_ + 1u);
    composeImages(scene, tick_, true);
    if (scene.objetBank == 9) {
        // Saved disassembly 0x11ef7..0x11f39: all three bases share AND 7,
        // then NOT / AND 3 for phases >= 4, with no shift or phase offset.
        const unsigned phase = tick_ & 7;
        const unsigned offset = phase < 4 ? phase : ((~phase) & 3);
        for (auto& object : scene.objects) {
            if (object.image >= 13 && object.image <= 16) object.image = 13 + offset;
            else if (object.image >= 19 && object.image <= 22) object.image = 19 + offset;
            else if (object.image >= 23 && object.image <= 26) object.image = 23 + offset;
        }
        return;
    }
    if (scene.objetBank == 4) {
        // Saved disassembly 0x11e0e..0x11e25: AND 7, fold, ADD 26 (no SHR).
        const unsigned phase = tick_ & 7;
        const std::size_t image = 26 + (phase < 4 ? phase : ((~phase) & 3));
        for (auto& object : scene.objects) {
            if (object.image >= 26 && object.image <= 29) object.image = image;
        }
        return;
    }
    if (scene.objetBank == 3) {
        // Saved disassembly 0x11e49..0x11e63: SHR 1, AND 7, fold, ADD 15.
        const unsigned phase = (tick_ >> 1) & 7;
        const std::size_t image = 15 + (phase < 4 ? phase : ((~phase) & 3));
        // Saved disassembly 0x11ec3..0x11ee4: rest outside phases 16..27.
        const unsigned pulse = tick_ & 31;
        const std::size_t pulseImage = pulse < 16 || pulse >= 28 ? 22
            : 22 + (pulse <= 22 ? pulse - 16 : 28 - pulse);
        for (auto& object : scene.objects) {
            if (object.image >= 15 && object.image <= 18) object.image = image;
            else if (object.image >= 22 && object.image <= 28) object.image = pulseImage;
        }
        return;
    }
    if (scene.objetBank != 2 || (tick_ & 1)) return;
    for (auto& object : scene.objects) {
        if (object.image >= 9 && object.image <= 11)
            object.image = object.image == 11 ? 9 : object.image + 1;
    }
}

void DecorativeAnimation::update(Scene& scene, double elapsedSeconds)
{
    if (!std::isfinite(elapsedSeconds) || elapsedSeconds <= 0) return;
    const double accumulated = remainder_ + elapsedSeconds;
    const double steps = std::floor(accumulated / stepSeconds + 1e-9);
    remainder_ = accumulated - steps * stepSeconds;
    if (remainder_ < 0) remainder_ = 0;
    // Do not reduce by the 16-bit period: each even step advances instance state.
    for (double remaining = steps; remaining >= 1; --remaining) step(scene);
}

} // namespace sprites
