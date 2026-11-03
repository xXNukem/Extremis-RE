#include "plane_renderer.h"
#include "plane_material.h"

#include <cmath>

namespace planes {
namespace {
bool valid(const wall_textures::Texture& texture)
{
    return texture.width > 0 && texture.height > 0 &&
           texture.pixels.size() == std::size_t(texture.width) * texture.height;
}

void renderRow(std::uint32_t* pixels, int width, Row row, const wall_textures::Texture& texture)
{
    for (int x = 0; x < width; ++x) {
        const auto uv = plane_material::coordinates(row.x + x * row.stepX, row.y + x * row.stepY,
                                                    texture.width, texture.height);
        const auto rgb = texture.pixels[std::size_t(uv.y) * texture.width + uv.x];
        pixels[x] = (std::uint32_t(rgb.r) << 24) | (std::uint32_t(rgb.g) << 16) |
                    (std::uint32_t(rgb.b) << 8) | 255;
    }
}
} // namespace

std::optional<Row> projectRow(const Camera& camera, const Projection& p, int y)
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

void render(std::vector<std::uint32_t>& pixels, const Camera& camera,
            const Projection& p, const wall_textures::PlaneBank& materials)
{
    if (p.width <= 0 || p.height <= 0 || pixels.size() != std::size_t(p.width) * p.height) return;
    for (int y = 0; y < p.height; ++y) {
        const auto row = projectRow(camera, p, y);
        if (!row) continue;
        const auto& texture = plane_material::select(materials, row->ceiling);
        if (valid(texture)) renderRow(pixels.data() + std::size_t(y) * p.width, p.width, *row, texture);
    }
}
} // namespace planes
