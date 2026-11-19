#include "vortex/render/first_person/scene_renderer.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
constexpr int width = 33, height = 25;
constexpr double focal = 20, eyeHeight = 12, ceilingHeight = 24;

struct Segment {
    vortex::Point start, end;
    std::uint16_t material;
};

std::size_t pixel(int x, int y) { return std::size_t(y) * width + x; }

void expect(bool condition, const char* failure)
{
    if (!condition) throw std::runtime_error(failure);
}

void renderScene()
{
    const vortex::CameraPose camera{};
    const vortex::Point origin{camera.x, camera.y};
    const std::array<Segment, 7> segments{{
        {{-96, -96}, {96, -96}, 2},
        {{96, -96}, {96, 96}, 2},
        {{96, 96}, {-96, 96}, 2},
        {{-96, 96}, {-96, -96}, 2},
        {{48, -34}, {48, 34}, 1},
        {{48, -34}, {60, -52}, 1},
        {{60, -52}, {76, -52}, 1},
    }};
    vortex::StaticRayGrid grid([&](auto addSegment) {
        for (const auto& segment : segments)
            addSegment(segment.start, segment.end);
    }, true); // Force indexing in this deliberately small scene.
    expect(grid.valid(), "Synthetic geometry must build an index");

    vortex::PreparedRayScene scene;
    for (const auto& segment : segments)
        scene.walls.push_back(vortex::prepareRaySurface(
            segment.start, segment.end, origin, segment.material, 1.0));

    namespace cpu = vortex::first_person;
    const cpu::View view{camera, {width, height, 1, 0, width / (2 * focal), focal, eyeHeight},
                         ceilingHeight, focal, scene, &grid};
    const vortex::Texture wall{2, 2, std::vector<vortex::Rgb>(4, {240, 20, 10})};
    std::vector<std::uint32_t> color;
    cpu::RenderBuffers buffers;
    const vortex::PlaneBank planes{
        {128, 128, std::vector<vortex::Rgb>(128 * 128, {32, 144, 32})},
        {128, 128, std::vector<vortex::Rgb>(128 * 128, {16, 32, 144})},
    };
    constexpr std::uint32_t ceilingColor = 0x102090ff;
    constexpr std::uint32_t floorColor = 0x209020ff;
    constexpr std::uint32_t wallColor = 0xe31209ff;
    const vortex::sprites::Bank sprites{{
        {{2, 2, std::vector<vortex::Rgb>(4, {255, 224, 0})}, 1, 1, {0, 1, 1, 1}},
    }};
    const std::vector<vortex::sprites::Instance> instances{
        {24, 0, 12, 12, 0},
        {72, 18, 12, 12, 0},
    };
    auto overlappingSprites = sprites;
    std::fill(overlappingSprites.images.front().texture.pixels.begin(),
              overlappingSprites.images.front().texture.pixels.end(), vortex::Rgb{0, 200, 255});
    const std::vector<vortex::sprites::Instance> overlapping{instances.front()};
    const cpu::SpriteLayer layers[] = {{&sprites, &instances}, {&overlappingSprites, &overlapping}};
    cpu::SceneStyle style;
    style.wallMaterial = [](const void* context, const vortex::RayHit& hit) {
        const auto* texture = static_cast<const vortex::Texture*>(context);
        const int x = std::min(texture->width - 1, int(hit.segmentPosition * texture->width));
        return cpu::WallSample{texture, x, {}};
    };
    style.wallContext = &wall;
    style.planes = &planes;
    style.planeRepeat = 640.0;
    style.layers = layers;
    style.layerCount = std::size(layers);
    expect(cpu::render(view, style, color, buffers), "First-person scene did not render");
    const auto& depth = buffers.depth;
    expect(buffers.wallDepthColumns.front() == 96 && buffers.wallDepthColumns[width / 2] == 48,
           "Room perimeter or interior structure did not occlude at the expected depth");
    expect(buffers.wallCoverage[width / 2] == std::pair{7, 17}, "Wall coverage changed");
    expect(color[pixel(0, 0)] == ceilingColor, "Ceiling not rasterized");
    expect(color[pixel(0, height - 1)] == floorColor, "Floor not rasterized");
    expect(color[pixel(10, 12)] == wallColor, "Covered wall pixels were overwritten");

    expect(color[pixel(16, 12)] == 0xffe000ff && depth[pixel(16, 12)] == 24,
           "First sprite layer did not retain its color and depth at an equal-depth overlap");
    expect(color[pixel(13, 9)] == wallColor && depth[pixel(13, 9)] == 48,
           "Transparent sprite texel overwrote the wall");
    expect(color[pixel(10, 12)] == wallColor && depth[pixel(10, 12)] == 48,
           "Wall did not hide the distant sprite");

    const auto unwarped = color;
    vortex::FisheyeEffect fisheye{{1.0, 0.20}};
    style.fisheye = &fisheye;
    expect(cpu::render(view, style, color, buffers) && color != unwarped,
           "Fisheye did not warp the framebuffer");
    expect(color.front() == ceilingColor && color.back() == floorColor &&
           color[pixel(width / 2, height / 2)] == unwarped[pixel(width / 2, height / 2)],
           "Fisheye moved an invariant sample");
}
}

int main()
{
    try {
        renderScene();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "Vortex CPU rendered a synthetic scene\n";
}
