#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <utility>
#include <vector>
#include "extremis/render/first_person/materials/plane_appearance.h"
#include "vortex/render/first_person/planes/plane_material.h"
#include "vortex/render/first_person/planes/plane_renderer.h"
#include "extremis/render/first_person/presentation/first_person_view.h"

#include <array>
#include <algorithm>
#include <SDL3/SDL.h>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void check(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}
bool near(double a, double b) { return std::abs(a - b) < 1e-8; }

void write(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes)
{
    std::ofstream stream(path, std::ios::binary);
    stream.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    check(bool(stream), "Cannot write plane test fixture");
}

void fixtures(const std::filesystem::path& root)
{
    std::filesystem::create_directories(root / "DATA");
    std::vector<std::uint8_t> pixels(32768), palette(768);
    for (int y = 0; y < 128; ++y) {
        for (int x = 0; x < 128; ++x) {
            pixels[y * 256 + x] = (x + 3 * y) % 64;
            pixels[y * 256 + 128 + x] = (3 * x + y + 16) % 64;
        }
    }
    for (int i = 0; i < 256; ++i) {
        palette[i * 3] = i % 64;
        palette[i * 3 + 1] = 63 - i % 64;
        palette[i * 3 + 2] = i % 64 / 2;
    }
    write(root / "DATA/SOL1.DAS", pixels);
    write(root / "DATA/PAL1.PAL", palette);
    write(root / "EXTREMIS.CFG", std::vector<std::uint8_t>(13));
}

void testProjection()
{
    const vortex::planes::Projection p{101, 81, 100, 128, 256};
    const auto row = vortex::planes::projectRow({0, 0, 0}, p, 80);
    check(row && !row->ceiling && near(row->depth, 320) && near(row->x, 320) &&
          near(row->y, 160) && near(row->stepX, 0) && near(row->stepY, -3.2),
          "Plane ray/position/screen-right projection incorrect");
    const auto rotated = vortex::planes::projectRow({20, 30, 3.141592653589793 / 2}, p, 80);
    check(rotated && near(rotated->x + 50 * rotated->stepX, 20) &&
          near(rotated->y + 50 * rotated->stepY, 350), "Plane projection ignores camera transform");
    const auto ceiling = vortex::planes::projectRow({0, 0, 0}, p, 0);
    check(ceiling && ceiling->ceiling && near(ceiling->depth, row->depth) &&
          near(ceiling->x, row->x) && near(ceiling->y, row->y), "Ceiling orientation mirrored");
    check(!vortex::planes::projectRow({}, p, 40), "Exact horizon caused invalid division");
    for (const int y : {39, 40}) {
        const auto horizon = vortex::planes::projectRow({}, {160, 80, 100, 128, 256}, y);
        check(horizon && std::isfinite(horizon->depth), "Near-horizon row is not finite");
    }
    check(!vortex::planes::projectRow({}, {0, 0, 0, 128, 256}, 0), "Invalid output size accepted");
}

void testCoordinates()
{
    const auto a = vortex::plane_material::coordinates(100, 200, 128, 128, wall_textures::solRepeatPeriod);
    const auto b = vortex::plane_material::coordinates(500, 600, 128, 128, wall_textures::solRepeatPeriod);
    const auto negative = vortex::plane_material::coordinates(-1, -401, 128, 128, wall_textures::solRepeatPeriod);
    check(a.x == 32 && a.y == 64 && a.x == b.x && a.y == b.y,
          "World axes, repeat period or aspect changed");
    check(negative.x == 127 && negative.y == 127, "Negative world wrapping failed");
}

void testDecodeAndSwitch(const std::filesystem::path& root)
{
    const auto bank = wall_textures::loadPlanes(root / "DATA", 1, 1);
    check(bank.floor.width == 128 && bank.ceiling.height == 128, "Incorrect SOL dimensions");
    for (int y = 0; y < 128; ++y) {
        for (int x = 0; x < 128; ++x) {
            const auto floor = bank.floor.pixels[y * 128 + x];
            const auto ceiling = bank.ceiling.pixels[y * 128 + x];
            check(floor.r == ((x + 3 * y) % 64 * 255 + 31) / 63 &&
                  ceiling.r == ((3 * x + y + 16) % 64 * 255 + 31) / 63,
                  "Interleaved SOL decode/palette/axes mismatch");
        }
    }
    PlaneAppearance appearance;
    appearance.load(root / "DATA", 1, 1);
    check(appearance.materials() && appearance.materials()->floor.pixels.size() == bank.floor.pixels.size(),
          "Level did not retain the decoded SOL materials");
}

void testRender(const wall_textures::PlaneBank& bank)
{
    for (const auto size : {SDL_Point{101, 81}, SDL_Point{160, 100}, SDL_Point{81, 161}}) {
        std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface(
            SDL_CreateSurface(size.x, size.y, SDL_PIXELFORMAT_RGBA32), SDL_DestroySurface);
        check(bool(surface), "Cannot create plane test surface");
        std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer(
            SDL_CreateSoftwareRenderer(surface.get()), SDL_DestroyRenderer);
        check(bool(renderer), "Cannot create plane test renderer");
        VideoRenderSettings video;
        FirstPersonRenderer view(renderer.get(),&video);
        game_lab::Level empty{};
        const auto pixel = [&](int y) {
            Uint8 r, g, b, a;
            check(SDL_ReadSurfacePixel(surface.get(), size.x / 2, y, &r, &g, &b, &a), "Pixel read failed");
            return std::uint32_t(r) << 24 | std::uint32_t(g) << 16 | std::uint32_t(b) << 8 | a;
        };
        check(view.draw(empty, {}, nullptr, nullptr, nullptr, &bank) && SDL_RenderPresent(renderer.get()),
              "Planes without wall textures failed");
        const auto textured = pixel(size.y - 1);
        video.preferences.floorAndCeiling = false;
        check(view.draw(empty, {}, nullptr, nullptr, nullptr, &bank) && SDL_RenderPresent(renderer.get()),
              "Disabling floor and ceiling did not reach the renderer");
        check(pixel(size.y - 1) == 0x2d2722ff && pixel(0) == 0x141720ff,
              "Disabled planes changed original flat colors");
        video.preferences.floorAndCeiling = true;
        check(view.draw(empty, {}, nullptr, nullptr, nullptr, &bank) && SDL_RenderPresent(renderer.get()) &&
              pixel(size.y - 1) == textured, "Texture restoration or resize failed");
        game_lab::Sector sector{};
        sector.vertices = {{500, 0}, {500, 200}};
        sector.walls = {{0, 1, 0x0001, 0}};
        empty.sectors.push_back(sector);
        const Camera camera{0, 100, 0};
        view.draw(empty, camera);
        SDL_RenderPresent(renderer.get());
        const auto wall = pixel(size.y / 2);
        check(view.draw(empty, camera, nullptr, nullptr, nullptr, &bank) &&
              SDL_RenderPresent(renderer.get()) && pixel(size.y / 2) == wall,
              "Planes overwrote foreground wall surface");
    }
}

void testRasterOrientation(const wall_textures::PlaneBank& bank)
{
    std::vector<std::uint32_t> pixels(101 * 81, 0x141720ff);
    vortex::planes::render(pixels, {}, {101, 81, 100, 128, 256}, bank, wall_textures::solRepeatPeriod);
    const auto expected = [](int index) {
        const auto channel = [](int v) { return std::uint32_t((v * 255 + 31) / 63); };
        return channel(index) << 24 | channel(63 - index) << 16 | channel(index / 2) << 8 | 255;
    };
    check(pixels[80 * 101 + 50] == expected(38) && pixels[50] == expected(2),
          "Floor/ceiling material selection or world U orientation incorrect");
    check(pixels[80 * 101] == expected(63) && pixels[80 * 101 + 100] == expected(10),
          "Horizontal-plane V axis mirrored or distorted");
    check(pixels[40 * 101 + 50] == 0x141720ff, "Exact horizon overwrote flat fallback");
}

// Give every texel a distinct color so an off-by-one UV cannot match by chance.
void testSampledRows()
{
    wall_textures::PlaneBank bank;
    for (auto* texture : {&bank.floor, &bank.ceiling}) {
        texture->width = texture->height = 128;
        for (int i = 0; i < 16384; ++i)
            texture->pixels.push_back({std::uint8_t(i), std::uint8_t(i >> 8),
                                       std::uint8_t(texture == &bank.floor)});
    }
    for (const Camera camera : {Camera{-400, 400, 0}, Camera{3.125, -3.125, 1.5707963267948966},
                                Camera{-400.0000000001, -27, -0.37}, Camera{1000100, 17, 0.64}}) {
        const vortex::planes::Projection p{841, 361, 400, 165, 256};
        std::vector<std::pair<int,int>> mask(p.width);
        for (int x = 0; x < p.width; ++x) mask[x] = {80 + x % 19, 270 - x % 23};
        std::vector<std::uint32_t> pixels(p.width * p.height, 0);
        vortex::planes::render(pixels, camera.renderPose(), p, bank, wall_textures::solRepeatPeriod, &mask);
        for (int y = 0; y < p.height; ++y) {
            const auto row = vortex::planes::projectRow(camera.renderPose(), p, y);
            for (int x = 0; x < p.width; ++x) {
                std::uint32_t expected = 0;
                if (row && (y < mask[x].first || y >= mask[x].second)) {
                    const auto uv = vortex::plane_material::coordinates(row->x + x * row->stepX,
                        row->y + x * row->stepY, 128, 128, wall_textures::solRepeatPeriod);
                    const unsigned index = unsigned(uv.y * 128 + uv.x);
                    expected = (index << 24) | ((index >> 8) << 16) |
                               (unsigned(!row->ceiling) << 8) | 255;
                }
                check(pixels[y * p.width + x] == expected, "Plane row differs from material sampler");
            }
        }
    }
}

void testConfiguredPeriod()
{
    wall_textures::PlaneBank bank;
    for (auto* texture : {&bank.floor, &bank.ceiling}) {
        texture->width = texture->height = 128;
        for (int y = 0; y < 128; ++y)
            for (int x = 0; x < 128; ++x)
                texture->pixels.push_back({std::uint8_t(x), std::uint8_t(y), 0});
    }
    // Both rows sample world X=16 and world Y=9,5,1,-3,-7. Assert independent
    // expected U/V for a period eligible for the fixed path and a smaller one.
    const vortex::planes::Projection projection{5, 2, 8, 2, 4};
    const vortex::CameraPose camera{-16, 1, 0};
    std::vector<std::uint32_t> pixels(10);
    vortex::planes::render(pixels, camera, projection, bank, 512);
    const std::array<std::uint32_t, 5> period512{{
        0x040200ff, 0x040100ff, 0x040000ff, 0x047f00ff, 0x047e00ff}};
    check(std::equal(pixels.begin(), pixels.begin() + 5, period512.begin()) &&
          std::equal(pixels.begin() + 5, pixels.end(), period512.begin()),
          "Configured 512-unit tile sampled incorrect texels");

    vortex::planes::render(pixels, camera, projection, bank, 64);
    const std::array<std::uint32_t, 5> period64{{
        0x201200ff, 0x200a00ff, 0x200200ff, 0x207a00ff, 0x207200ff}};
    check(std::equal(pixels.begin(), pixels.begin() + 5, period64.begin()) &&
          std::equal(pixels.begin() + 5, pixels.end(), period64.begin()),
          "Configured 64-unit tile sampled incorrect texels");

    const auto seam = vortex::plane_material::coordinates(-1e-10, -1e-6, 128, 128, 64);
    check(seam.x == 0 && seam.y == 127, "Small-period seam guard is incorrect");
    const auto valid = pixels;
    vortex::planes::render(pixels, camera, projection, bank, 0);
    check(pixels == valid, "Invalid period overwrote the framebuffer");
}

void testInvalidResources(const std::filesystem::path& root)
{
    for (const std::size_t size : {0u, 32767u, 32769u}) {
        write(root / "DATA/SOL1.DAS", std::vector<std::uint8_t>(size));
        bool rejected = false;
        try { wall_textures::loadPlanes(root / "DATA", 1, 1); }
        catch (const std::runtime_error&) { rejected = true; }
        check(rejected, "Invalid SOL size accepted");
    }
    PlaneAppearance appearance;
    appearance.load(root / "DATA", 1, 1);
    check(!appearance.materials(), "Invalid SOL did not fall back to flat colors");
}
} // namespace

int main(int argc, char* argv[])
{
    if (argc != 2) return 1;
    try {
        const std::filesystem::path root = argv[1];
        fixtures(root);
        testProjection();
        testSampledRows();
        testConfiguredPeriod();
        testCoordinates();
        testDecodeAndSwitch(root);
        const auto bank = wall_textures::loadPlanes(root / "DATA", 1, 1);
        testRasterOrientation(bank);
        testRender(bank);
        testInvalidResources(root);
        std::cout << "SOL decoding, plane projection, axes, horizon, F, restoration and resize passed.\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
