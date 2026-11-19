#include "plane_sampling.h"

#include <cmath>
#include <cstdint>
#include <algorithm>

namespace vortex::plane_material {
namespace {
int coordinate(double world, int size, const Sampling& sampling)
{
    // VORTEX_PLANE_UV_FASTPATH_V1: Index 128×128 textures directly
    // instead of performing fmod for every pixel.
    // Fall back at texel boundaries so rounding and the original seam fix
    // remain pixel-identical, including for negative world coordinates.
    if (size == 128 && std::abs(world) < sampling.fastPathLimit) {
        const double scaled = world * sampling.texelsPerUnit;
        const auto cell = static_cast<std::int64_t>(std::floor(scaled));
        const double fraction = scaled - double(cell);
        if (fraction > 1e-9 && fraction < 1.0 - 1e-9)
            return int(static_cast<std::uint64_t>(cell) & 127u);
    }
    double wrapped = std::fmod(world, sampling.period);
    if (wrapped < 0) {
        wrapped += sampling.period;
        if (wrapped == sampling.period) return 0;
    }
    // Pixel-ray cancellation at a tile edge can leave a tiny negative coordinate.
    if (wrapped < sampling.seam || wrapped > sampling.period - sampling.seam) wrapped = 0;
    return std::clamp(int(wrapped / sampling.period * size), 0, size - 1);
}
} // namespace

Sampling makeSampling(double repeatPeriod)
{
    // The fast paths were validated for magnitudes below 320000 texels.
    // At periods of at least 256 units, the seam consumes at most half of
    // the direct path's 1e-9 texel guard. Smaller periods use modulo.
    const bool fastPath = repeatPeriod >= 256.0;
    return {repeatPeriod, std::min(1e-9, repeatPeriod * (1e-9 / 256.0)),
            fastPath ? 128.0 / repeatPeriod : 0.0,
            fastPath ? repeatPeriod * 2500.0 : 0.0};
}

Texel coordinates(double worldX, double worldY, int width, int height, const Sampling& sampling)
{
    if (width <= 0 || height <= 0 || !std::isfinite(worldX) || !std::isfinite(worldY)) return {0, 0};
    return {coordinate(worldX, width, sampling), coordinate(worldY, height, sampling)};
}

Texel coordinates(double worldX, double worldY, int width, int height, double repeatPeriod)
{
    if (!(repeatPeriod > 0.0) || !std::isfinite(repeatPeriod)) return {0, 0};
    return coordinates(worldX, worldY, width, height, makeSampling(repeatPeriod));
}

const Texture& select(const PlaneBank& bank, bool ceiling)
{
    return ceiling ? bank.ceiling : bank.floor;
}
} // namespace vortex::plane_material
