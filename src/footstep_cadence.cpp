#include "footstep_cadence.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace audio {

std::uint64_t FootstepCadence::advance(double effectiveDistance)
{
    if (!std::isfinite(effectiveDistance) || effectiveDistance <= 0) return 0;
    const double total = distance_ + effectiveDistance;
    // ceil(144 / 16) * 10 initially, then ceil((144 - 28) / 16) * 10.
    const double threshold = first_ ? 90.0 : 80.0;
    if (total + 1e-9 < threshold) {
        distance_ = total;
        return 0;
    }
    const double beyond = std::max(0.0, total - threshold);
    const double additional = std::floor(beyond / 80.0 + 1e-9);
    distance_ = std::max(0.0, beyond - additional * 80.0);
    first_ = false;
    const double count = additional + 1;
    if (count >= double(std::numeric_limits<std::uint64_t>::max()))
        return std::numeric_limits<std::uint64_t>::max();
    return static_cast<std::uint64_t>(count);
}

} // namespace audio
