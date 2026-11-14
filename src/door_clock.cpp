#include "door_clock.h"

namespace doors::dos {
void Counter::advance(Time now)
{
    if (now <= previous_) return;
    const auto ns = static_cast<std::uint64_t>((now - previous_).count());
    previous_ = now;
    // Nominal PC PIT input, also used in weapon_fire.cpp; EXE divisor 7457.
    constexpr std::uint64_t frequency = 1193182;
    constexpr std::uint64_t period = 7457ULL * 1000000000;
    // Split before multiplying: even a nanoseconds::max interval fits uint64_t.
    const auto scaled = (ns % period) * frequency + fraction_;
    const auto ticks = (ns / period) * frequency + scaled / period;
    fraction_ = scaled % period;
    // The PIT keeps its phase while [0x142c] inhibits counter increments.
    if (!inhibited_) value_ = static_cast<std::uint16_t>(value_ + ticks);
}

void Counter::inhibit(bool inhibited, Time now)
{
    advance(now);
    inhibited_ = inhibited;
}
} // namespace doors::dos
