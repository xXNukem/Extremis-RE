#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace vga_palette {
using Rgb8 = std::array<std::uint8_t, 3>;
using Palette = std::array<Rgb8, 256>;

// Expand one complete 768-byte RGB palette to full-range RGB8.
Palette decode(const std::uint8_t* bytes, std::string_view invalidComponent);
}
