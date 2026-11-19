#pragma once

#include <cstdint>

namespace extremis::pit {
inline constexpr std::uint64_t inputHz = 1193182;
inline constexpr std::uint64_t divisor = 7457;
inline constexpr double rate = double(inputHz) / double(divisor);
}
