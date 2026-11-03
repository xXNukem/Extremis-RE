#include "sprite_renderer.h"

#include <algorithm>
#include <cmath>

namespace sprites {
namespace {
bool valid(const Image& image)
{
    const auto& texture = image.texture;
    return texture.width > 0 && texture.height > 0 &&
        texture.pixels.size() == std::size_t(texture.width) * texture.height &&
        image.indices.size() == texture.pixels.size();
}

int clippedEdge(double value, int limit)
{
    return int(std::ceil(std::clamp(value - 0.5, 0.0, double(limit))));
}

void drawObject(std::vector<std::uint32_t>& pixels, std::vector<double>& depth,
                const Instance& object, const Image& image, const Rectangle& rect,
                const Projection& projection)
{
    const auto& texture = image.texture;
    const int x0 = clippedEdge(rect.left, projection.width);
    const int x1 = clippedEdge(rect.left + rect.width, projection.width);
    const int y0 = clippedEdge(rect.top, projection.height);
    const int y1 = clippedEdge(rect.top + rect.height, projection.height);
    for (int y = y0; y < y1; ++y) {
        int v = std::clamp(int((y + 0.5 - rect.top) * texture.height / rect.height),
                           0, texture.height - 1);
        if (object.type & 0x0100) v = texture.height - 1 - v;
        for (int x = x0; x < x1; ++x) {
            const auto target = std::size_t(y) * projection.width + x;
            if (rect.depth >= depth[target]) continue;
            int u = std::clamp(int((x + 0.5 - rect.left) * texture.width / rect.width),
                               0, texture.width - 1);
            if (object.type & 0x0200) u = texture.width - 1 - u;
            const auto source = std::size_t(v) * texture.width + u;
            if (image.indices[source] == 0) continue;
            const auto c = texture.pixels[source];
            pixels[target] = (std::uint32_t(c.r) << 24) | (std::uint32_t(c.g) << 16) |
                             (std::uint32_t(c.b) << 8) | 255;
            depth[target] = rect.depth;
        }
    }
}
} // namespace

std::optional<Rectangle> project(const Instance& object, const Image& image,
                               const Camera& camera, const Projection& p)
{
    if (!valid(image) || p.width <= 0 || p.height <= 0 || p.focal <= 0 ||
        !std::isfinite(object.height) || object.height <= 0) return std::nullopt;
    const double dx = object.x - camera.x, dy = object.y - camera.y;
    const double c = std::cos(camera.angle), s = std::sin(camera.angle);
    const double depth = dx * c + dy * s;
    if (!std::isfinite(depth) || depth < 0.01) return std::nullopt;
    const double scale = object.height * p.focal / (depth * image.texture.height);
    const double anchorX = (object.type & 0x0200)
        ? image.texture.width - image.anchorX : image.anchorX;
    const double anchorY = (object.type & 0x0100)
        ? image.texture.height - image.anchorY : image.anchorY;
    const double left = p.width * 0.5 + (dx * s - dy * c) * p.focal / depth - anchorX * scale;
    const double top = p.height * 0.5 - (object.z - p.eyeHeight) * p.focal / depth - anchorY * scale;
    if (!std::isfinite(left) || !std::isfinite(top) || !std::isfinite(scale)) return std::nullopt;
    return Rectangle{left, top, image.texture.width * scale, image.texture.height * scale, depth};
}

void render(std::vector<std::uint32_t>& pixels, std::vector<double>& depth,
            const Scene& scene, const Camera& camera, const Projection& projection)
{
    render(pixels,depth,scene.bank,scene.objects,camera,projection);
}
void render(std::vector<std::uint32_t>& pixels, std::vector<double>& depth,
            const Bank& bank, const std::vector<Instance>& objects, const Camera& camera,
            const Projection& projection)
{
    if (projection.width <= 0 || projection.height <= 0 ||
        pixels.size() != std::size_t(projection.width) * projection.height ||
        depth.size() != pixels.size()) return;
    for (const auto& object : objects) {
        if (object.image >= bank.images.size()) continue;
        const auto& image = bank.images[object.image];
        const auto rectangle = project(object, image, camera, projection);
        if (rectangle) drawObject(pixels, depth, object, image, *rectangle, projection);
    }
}
} // namespace sprites
