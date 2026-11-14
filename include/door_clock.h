#pragma once

#include <chrono>
#include <cstdint>

namespace doors::dos {
// Isolated [0x1428] counter. This does not schedule DOS main-loop iterations.
class Counter {
public:
    using Clock = std::chrono::steady_clock;
    using Time = std::chrono::time_point<Clock, std::chrono::nanoseconds>;
    explicit Counter(Time now = Clock::now()) : previous_(now) {}
    void advance(Time now = Clock::now());
    // Account for the preceding interval under the old gate before changing it.
    void inhibit(bool inhibited, Time now = Clock::now());
    std::uint16_t value() const { return value_; }
    // Pure word write: call advance first if time has elapsed. PIT phase survives.
    void reset() { value_ = 1; }

private:
    Time previous_;
    std::uint64_t fraction_ = 0;
    std::uint16_t value_ = 1;
    bool inhibited_ = false;
};
} // namespace doors::dos
