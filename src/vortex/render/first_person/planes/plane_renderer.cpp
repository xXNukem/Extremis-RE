#include "vortex/render/first_person/planes/plane_renderer.h"

#include "plane_sampling.h"

#include <cmath>

namespace vortex::planes {
namespace {
void renderRow(std::uint32_t* pixels, int width, int y, Row row,
               const Texture& texture, const plane_material::Sampling& sampling,
               const std::vector<std::pair<int,int>>* wallCoverage)
{
    // A 32.32 texel accumulator leaves
    // less than one low bit of truncation error per column (including origin).
    // Keep both axes inside the material sampler's normal coordinate range.
    const bool fixedPoint = texture.width == 128 && texture.height == 128 && width <= 8192 &&
        std::abs(row.x) + width * std::abs(row.stepX) < sampling.fastPathLimit &&
        std::abs(row.y) + width * std::abs(row.stepY) < sampling.fastPathLimit;
    const double scale = sampling.texelsPerUnit * 4294967296.0;
    const auto fixed = [scale](double v) { return std::uint64_t(std::int64_t(v * scale)); };
    std::uint64_t u = 0, v = 0, du = 0, dv = 0;
    if (fixedPoint) {
        u = fixed(row.x);
        v = fixed(row.y);
        du = fixed(row.stepX);
        dv = fixed(row.stepY);
        if (row.stepX == 0 || row.stepY == 0) {
            // Sample constant axes once, including exact seams.
            const auto uv = plane_material::coordinates(row.x, row.y, 128, 128, sampling);
            if (row.stepX == 0) u = (std::uint64_t(uv.x) << 32) + 0x80000000u;
            if (row.stepY == 0) v = (std::uint64_t(uv.y) << 32) + 0x80000000u;
        }
    }
    // In low-word units, width + 1 covers accumulated truncation.
    // Another 16 covers double rounding and the sampler's 1e-9 texel guard.
    const unsigned guard = unsigned(width) + 16;
    const auto* source = texture.pixels.data();
    const auto* coverage = wallCoverage ? wallCoverage->data() : nullptr;
    for (int x = 0; x < width; ++x, u += du, v += dv) {
        if (coverage && y >= coverage[x].first && y < coverage[x].second) continue;
        plane_material::Texel uv;
        if (fixedPoint && std::uint32_t(u + guard) > 2 * guard &&
                          std::uint32_t(v + guard) > 2 * guard) {
            uv = {int((u >> 32) & 127), int((v >> 32) & 127)};
        } else {
            uv = plane_material::coordinates(row.x + x * row.stepX, row.y + x * row.stepY,
                                             texture.width, texture.height, sampling);
        }
        const auto rgb = source[uv.y * std::size_t(texture.width) + uv.x];
        pixels[x] = (std::uint32_t(rgb.r) << 24) | (std::uint32_t(rgb.g) << 16) |
                    (std::uint32_t(rgb.b) << 8) | 255;
    }
}
} // namespace

std::optional<Row> projectRow(const CameraPose& camera, const Projection& p, int y)
{
    if (p.width <= 0 || p.height <= 0 || !std::isfinite(p.focal) ||
        p.focal <= 0 || y < 0 || y >= p.height) return std::nullopt;
    const double offset = y + 0.5 - p.height * 0.5;
    if (std::abs(offset) < 1e-9) return std::nullopt; // Pixel center exactly on the horizon.
    const bool ceiling = offset < 0;
    const double elevation = ceiling ? p.ceilingHeight - p.eyeHeight : p.eyeHeight;
    if (elevation <= 0) return std::nullopt;
    const double depth = elevation * p.focal / std::abs(offset);
    if (!std::isfinite(depth)) return std::nullopt;
    const double side = (0.5 - p.width * 0.5) / p.focal;
    const double forwardX = std::cos(camera.angle), forwardY = std::sin(camera.angle);
    const double step = depth / p.focal;
    return Row{camera.x + depth * (forwardX + forwardY * side),
               camera.y + depth * (forwardY - forwardX * side),
               forwardY * step, -forwardX * step, depth, ceiling};
}

void render(std::vector<std::uint32_t>& pixels, const CameraPose& camera,
            const Projection& p, const PlaneBank& materials, double repeatPeriod,
            const std::vector<std::pair<int,int>>* wallCoverage)
{
    if (p.width <= 0 || p.height <= 0 || pixels.size() != std::size_t(p.width) * p.height) return;
    if (wallCoverage && wallCoverage->size() != std::size_t(p.width)) return;
    if (!(repeatPeriod > 0.0) || !std::isfinite(repeatPeriod)) return;
    const auto sampling = plane_material::makeSampling(repeatPeriod);
    for (int y = 0; y < p.height; ++y) {
        const auto row = projectRow(camera, p, y);
        if (!row) continue;
        const auto& texture = plane_material::select(materials, row->ceiling);
        if (validTexture(texture)) renderRow(pixels.data() + std::size_t(y) * p.width,
                                       p.width, y, *row, texture, sampling, wallCoverage);
    }
}
} // namespace vortex::planes
