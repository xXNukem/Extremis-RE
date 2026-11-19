#pragma once

#include <cstdint>

namespace vortex {
struct Rgb {
    std::uint8_t r, g, b;
};

// Compose byte-sized channels in 0xRRGGBBFF order. The shifts do not mask inputs.
constexpr std::uint32_t packOpaqueRgb(std::uint32_t red, std::uint32_t green, std::uint32_t blue)
{
    return (red << 24) | (green << 16) | (blue << 8) | 255u;
}
} // namespace vortex
