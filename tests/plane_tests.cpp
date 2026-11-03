#include "plane_appearance.h"
#include "plane_material.h"
#include "plane_renderer.h"
#include "first_person_view.h"

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
    const planes::Projection p{101, 81, 100, 128, 256};
    const auto row = planes::projectRow({0, 0, 0}, p, 80);
    check(row && !row->ceiling && near(row->depth, 320) && near(row->x, 320) &&
          near(row->y, 160) && near(row->stepX, 0) && near(row->stepY, -3.2),
          "Plane ray/position/screen-right projection incorrect");
    const auto rotated = planes::projectRow({20, 30, 3.141592653589793 / 2}, p, 80);
    check(rotated && near(rotated->x + 50 * rotated->stepX, 20) &&
          near(rotated->y + 50 * rotated->stepY, 350), "Plane projection ignores camera transform");
    const auto ceiling = planes::projectRow({0, 0, 0}, p, 0);
    check(ceiling && ceiling->ceiling && near(ceiling->depth, row->depth) &&
          near(ceiling->x, row->x) && near(ceiling->y, row->y), "Ceiling orientation mirrored");
    check(!planes::projectRow({}, p, 40), "Exact horizon caused invalid division");
    for (const int y : {39, 40}) {
        const auto horizon = planes::projectRow({}, {160, 80, 100, 128, 256}, y);
        check(horizon && std::isfinite(horizon->depth), "Near-horizon row is not finite");
    }
    check(!planes::projectRow({}, {0, 0, 0, 128, 256}, 0), "Invalid output size accepted");
}

void testCoordinates()
{
    const auto a = plane_material::coordinates(100, 200, 128, 128);
    const auto b = plane_material::coordinates(500, 600, 128, 128);
    const auto negative = plane_material::coordinates(-1, -401, 128, 128);
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
    check(!appearance.enabled() && !appearance.materials(), "Original CFG off state ignored");
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.windowID = 31;
    event.key.scancode = SDL_SCANCODE_F;
    check(appearance.handleEvent(event, 31) && appearance.materials(), "F did not enable both planes");
    const auto* original = appearance.materials();
    event.key.repeat = true;
    check(!appearance.handleEvent(event, 31) && appearance.materials() == original, "F repeat toggled state");
    event.key.repeat = false;
    check(!appearance.handleEvent(event, 32), "Wrong-window F toggles state");
    appearance.handleEvent(event, 31);
    check(!appearance.enabled() && !appearance.materials(), "F did not restore flat mode");
    appearance.handleEvent(event, 31);
    check(appearance.materials() == original, "Re-enabling F replaced/lost original materials");
    event.type = SDL_EVENT_KEY_UP;
    check(!appearance.handleEvent(event, 31), "F release toggled state");
    std::filesystem::remove(root / "EXTREMIS.CFG");
    appearance.load(root / "DATA", 1, 1);
    check(appearance.enabled() && appearance.materials(), "Missing CFG provisional enabled default failed");
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
        FirstPersonRenderer view(renderer.get());
        game_lab::Level empty{};
        const auto pixel = [&](int y) {
            Uint8 r, g, b, a;
            check(SDL_ReadSurfacePixel(surface.get(), size.x / 2, y, &r, &g, &b, &a), "Pixel read failed");
            return std::uint32_t(r) << 24 | std::uint32_t(g) << 16 | std::uint32_t(b) << 8 | a;
        };
        check(view.draw(empty, {}, nullptr, nullptr, nullptr, &bank) && SDL_RenderPresent(renderer.get()),
              "Planes without wall textures failed");
        const auto textured = pixel(size.y - 1);
        check(view.draw(empty, {}) && SDL_RenderPresent(renderer.get()), "Flat plane fallback failed");
        check(pixel(size.y - 1) == 0x2d2722ff && pixel(0) == 0x141720ff,
              "Disabled planes changed original flat colors");
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
    planes::render(pixels, {}, {101, 81, 100, 128, 256}, bank);
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
