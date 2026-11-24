#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace game_lab { struct Level; }

namespace extremis {
struct PaletteTint {
    unsigned percent = 0;
    std::array<unsigned, 3> dac{};
    bool optics = false;
};

PaletteTint lampTint(const game_lab::Level& level, unsigned sector, double x, double y, bool enabled = true);
void tintFrame(const std::uint32_t* source, std::uint32_t* output, std::size_t count, PaletteTint tint);

class PowerSurge {
public:
    bool update(std::uint16_t ticks, bool higherPriorityEffect = false);
    PaletteTint tint() const;
private:
    unsigned randomWord();
    std::int32_t remaining_ = 9600;
    std::uint32_t random_ = 0x7481c5d3u;
    unsigned darkness_ = 0;
};

PaletteTint ambientTint(PaletteTint lamp, PaletteTint transient, bool optics = false);
}
