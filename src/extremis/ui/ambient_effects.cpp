#include <cstddef>
#include <array>
#include <cstdint>
#include "extremis/ui/ambient_effects.h"
#include "extremis/resources/game_lab.h"

#include <algorithm>
#include <cmath>

namespace extremis {
PaletteTint lampTint(const game_lab::Level& level, unsigned index, double x, double y, bool enabled)
{
    if (!enabled || level.objetBank != 1 || index >= level.sectors.size()) return {};
    const auto& sector = level.sectors[index];
    for (const auto& entity : sector.entitiesA) {
        const auto image = entity.type & 255u;
        if ((entity.type & 0x3000) || (image != 2 && image != 3) ||
            entity.vertex >= sector.vertices.size()) continue;
        const auto vertex = sector.vertices[entity.vertex];
        const double distance = std::max(std::abs(std::trunc(x) - sector.gx - vertex.x),
                                         std::abs(std::trunc(y) - sector.gy - vertex.y));
        if (distance < 160) {
            return {80u - static_cast<unsigned>(distance) / 2,
                    image == 2 ? std::array<unsigned, 3>{63, 0, 0}
                               : std::array<unsigned, 3>{0, 63, 0}};
        }
    }
    return {};
}

void tintFrame(const std::uint32_t* source, std::uint32_t* output, std::size_t count, PaletteTint tint)
{
    if (!tint.percent && !tint.optics) {
        std::copy_n(source, count, output);
        return;
    }
    const auto amount = std::min(tint.percent, 100u);
    const auto scale = ((100 - amount) * 65536u) / 100;
    constexpr unsigned shifts[]{24, 16, 8};
    std::array<std::array<std::uint32_t, 256>, 3> table{};
    for (unsigned channel = 0; channel < 3; ++channel) {
        const auto target = tint.dac[channel] * amount / 100;
        for (unsigned value = 0; value < 256; ++value) {
            auto dac = (value * 63 + 127) / 255;
            if (tint.optics) dac = channel==1 ? std::min(63u,dac*2) : 0;
            const auto mixed = ((dac * scale) >> 16) + target;
            table[channel][value] = ((mixed * 255 + 31) / 63) << shifts[channel];
        }
    }
    for (std::size_t i = 0; i < count; ++i) {
        const auto color = source[i];
        output[i] = table[0][color >> 24] | table[1][(color >> 16) & 255] |
                    table[2][(color >> 8) & 255] | (color & 255);
    }
}

unsigned PowerSurge::randomWord()
{
    // Independent host PRNG. The DOS executable shares its RNG with other systems.
    random_ ^= random_ << 13;
    random_ ^= random_ >> 17;
    random_ ^= random_ << 5;
    return random_ & 65535u;
}

bool PowerSurge::update(std::uint16_t ticks, bool higherPriorityEffect)
{
    remaining_ -= ticks;
    const bool active = remaining_ < 0;
    darkness_ = active && !higherPriorityEffect ? (randomWord() & 63u) + 30u : 0;
    if (remaining_ < -640) remaining_ = 9600 + randomWord();
    return active;
}

PaletteTint PowerSurge::tint() const { return {darkness_, {0, 0, 0}}; }

PaletteTint ambientTint(PaletteTint lamp, PaletteTint transient, bool optics)
{
    auto tint=transient.percent ? transient : optics ? PaletteTint{} : lamp;
    tint.optics=optics;
    return tint;
}
}
