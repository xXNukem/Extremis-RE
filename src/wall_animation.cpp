#include "wall_animation.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace wall_animation {
namespace {
// EXTREMIS.EXE file 0x1524f..0x1526e, used at 0x151f2.
constexpr std::array<unsigned, 32> indicatorMasks{
    0,1,2,4,8,16,32,64,32,16,8,4,2,1,0,1,
    3,7,15,31,63,127,63,31,15,7,3,1,0,127,0,127
};

void copyOverlay(wall_textures::Texture& target, const wall_textures::Texture& source,
                 int x, int y)
{
    if (x < 0 || y < 0 || source.width <= 0 || source.height <= 0 ||
        x + source.width > target.width || y + source.height > target.height ||
        source.pixels.size() != std::size_t(source.width * source.height) ||
        target.pixels.size() != std::size_t(target.width * target.height)) {
        throw std::runtime_error("Wall animation overlay outside texture");
    }
    for (int row = 0; row < source.height; ++row) {
        std::copy_n(source.pixels.begin() + row * source.width, source.width,
                    target.pixels.begin() + (y + row) * target.width + x);
    }
}
} // namespace

bool isAnimated(std::size_t material)
{
    return material >= 11 && material <= 13;
}

std::array<Overlay, 10> overlays(std::uint16_t tick)
{
    std::array<Overlay, 10> result{{
        {12, std::size_t(4 + (tick & 7)), 10, 25},
        {12, std::size_t(12 + tick % 6), 36, 25},
        {13, std::size_t((tick >> 5) & 3), 11, 17}
    }};
    const unsigned mask = indicatorMasks[tick & 31];
    for (unsigned row = 0; row < 7; ++row) {
        // Bit 5 reverses the direction. The original pre-shifts AL once and
        // then tests carry after each SHL, giving bits 6,5,...,0.
        const unsigned bit = (tick & 32) ? 6 - row : row;
        result[3 + row] = {11, (mask & (1u << bit)) ? 19u : 18u,
                           12, 14 + int(row) * 7};
    }
    return result;
}

void compose(wall_textures::Bank& materials, const wall_textures::Bank& images,
             std::uint16_t tick)
{
    if (materials.textures.size() < 14 || images.textures.size() != 20) {
        throw std::runtime_error("Wall animation requires DALLE1 and ADALLE");
    }
    for (const auto& overlay : overlays(tick)) {
        copyOverlay(materials.textures[overlay.material], images.textures[overlay.image],
                    overlay.x, overlay.y);
    }
}

bool Clock::advance(double elapsedSeconds)
{
    if (!std::isfinite(elapsedSeconds) || elapsedSeconds <= 0) return false;
    const auto previous = tick();
    constexpr double cycle = 65536 * provisionalTickSeconds;
    phase_ = std::fmod(phase_ + std::fmod(elapsedSeconds, cycle), cycle);
    return previous != tick();
}

std::uint16_t Clock::tick() const
{
    // Small tolerance prevents frame partition rounding from delaying a boundary.
    return static_cast<std::uint16_t>(
        static_cast<unsigned>(std::floor(phase_ / provisionalTickSeconds + 1e-9)) & 65535);
}
} // namespace wall_animation
