#include "first_person_view.h"
#include "wall_material.h"

#include <SDL3/SDL.h>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void check(bool ok, const char* message)
{
    if (!ok) throw std::runtime_error(message);
}

void testTextureSampling()
{
    game_lab::Level level{};
    game_lab::Sector sector{};
    sector.vertices = {{100, 0}, {100, 100}};
    sector.walls = {{0, 1, 0, 0}};
    level.sectors.push_back(sector);
    const Camera camera{50, 50, 0};
    wall_textures::Texture texture{4, 60, {}};
    for (int y = 0; y < texture.height; ++y) {
        for (int x = 0; x < texture.width; ++x) {
            texture.pixels.push_back({std::uint8_t(y * 4), std::uint8_t(x * 60), 17});
        }
    }
    check(castWallRay(level, camera, 1, 1).segmentPosition == 1 &&
          castWallRay(level, camera, 1, -1).segmentPosition == 0,
          "Endpoint intersection fractions incorrect");
    for (const auto size : {SDL_Point{101, 81}, SDL_Point{161, 101}, SDL_Point{101, 161}}) {
        std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface(
            SDL_CreateSurface(size.x, size.y, SDL_PIXELFORMAT_RGBA32), SDL_DestroySurface);
        check(bool(surface), "Texture test surface failed");
        std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer(
            SDL_CreateSoftwareRenderer(surface.get()), SDL_DestroyRenderer);
        check(bool(renderer), "Texture test renderer failed");
        FirstPersonRenderer view(renderer.get());
        check(view.draw(level, camera, &texture) && SDL_RenderPresent(renderer.get()),
              "Textured rendering failed");
        const double focal = size.x / (2 * std::tan(70.0 * 3.14159265358979323846 / 360));
        const double top = size.y * 0.5 - 128 * focal / 50;
        const double wallPixels = 256 * focal / 50;
        const double shade = 0.2 + 0.8 / (1 + 50.0 / 700);
        for (const int x : {0, size.x / 2, size.x - 1}) {
            const double offset = (2 * (x + 0.5) / size.x - 1) *
                                  std::tan(70.0 * 3.14159265358979323846 / 360);
            const double u = (50 - 50 * offset) / 100;
            const int texX = int(u * texture.width);
            for (const int y : {0, size.y / 2, size.y - 1}) {
                const int texY = int((y + 0.5 - top) / wallPixels * texture.height);
                Uint8 r = 0, g = 0, b = 0, a = 0;
                check(SDL_ReadSurfacePixel(surface.get(), x, y, &r, &g, &b, &a) &&
                      r == Uint8(texY * 4 * shade) && g == Uint8(texX * 60 * shade) &&
                      b == Uint8(17 * shade) && a == 255,
                      "Perspective / clipped texture sampling mismatch");
            }
        }
        check(view.draw(level, camera) && SDL_RenderPresent(renderer.get()),
              "Flat fallback failed");
        const Camera farCamera{-1000, 50, 0};
        check(view.draw(level, farCamera, &texture) && SDL_RenderPresent(renderer.get()),
              "Distant textured wall failed");
        Uint8 r = 0, g = 0, b = 0, a = 0;
        check(SDL_ReadSurfacePixel(surface.get(), 0, 0, &r, &g, &b, &a) &&
              r == 20 && g == 23 && b == 32, "Ceiling color changed");
        check(SDL_ReadSurfacePixel(surface.get(), 0, size.y - 1, &r, &g, &b, &a) &&
              r == 45 && g == 39 && b == 34, "Floor color changed");
        wall_textures::Texture invalid{};
        check(!view.draw(level, camera, &invalid), "Invalid texture accepted");
    }
}

void testWallMaterials()
{
    check(wallTextureIndex(0x0001, 54) == 1 && wallTextureIndex(0x2003, 54) == 3 &&
          wallTextureIndex(0x3104, 54) == 4 && wallTextureIndex(0x3205, 54) == 5 &&
          wallTextureIndex(0x3508, 54) == 8 && wallTextureIndex(0x0000, 54) == 0,
          "Verified zero-based low-byte mapping failed");
    check(!wallTextureIndex(0x1000, 54) && !wallTextureIndex(0x1809, 54) &&
          !wallTextureIndex(0x0036, 54) && !wallTextureIndex(0x00ff, 54) &&
          !wallTextureIndex(0x4001, 54) && !wallTextureIndex(0, 0),
          "Portal or invalid material index accepted");

    game_lab::Level level{};
    game_lab::Sector sector{};
    sector.vertices = {{100, 0}, {100, 50}, {100, 100}, {75, 0}, {75, 100}};
    sector.walls = {{0, 1, 0x0001, 0xffff}, {1, 2, 0x3205, 263}, {3, 4, 0x1809, 0}};
    level.sectors.push_back(sector);
    wall_textures::Bank bank;
    bank.textures.resize(6, wall_textures::Texture{1, 1, {{0, 0, 0}}});
    bank.textures[1].pixels[0] = {255, 0, 0};
    bank.textures[5].pixels[0] = {0, 0, 255};
    const wall_textures::Texture overrideTexture{1, 1, {{0, 255, 0}}};
    std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface(
        SDL_CreateSurface(101, 81, SDL_PIXELFORMAT_RGBA32), SDL_DestroySurface);
    check(bool(surface), "Material test surface failed");
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer(
        SDL_CreateSoftwareRenderer(surface.get()), SDL_DestroyRenderer);
    check(bool(renderer), "Material test renderer failed");
    FirstPersonRenderer view(renderer.get());
    const Camera camera{50, 50, 0};
    const auto draw = [&](const wall_textures::Texture* overrideImage = nullptr) {
        check(view.draw(level, camera, overrideImage, &bank) && SDL_RenderPresent(renderer.get()),
              "Material renderer failed");
    };
    const auto pixel = [&](int x) {
        wall_textures::Rgb rgb{};
        Uint8 alpha = 0;
        check(SDL_ReadSurfacePixel(surface.get(), x, 40, &rgb.r, &rgb.g, &rgb.b, &alpha),
              "Cannot inspect material pixel");
        return rgb;
    };
    draw();
    check(pixel(0).b > 0 && pixel(0).r == 0 && pixel(100).r > 0 && pixel(100).b == 0,
          "Per-wall material selection, extra handling or portal visibility failed");
    draw(&overrideTexture);
    check(pixel(0).g > 0 && pixel(0).b == 0 && pixel(100).g > 0 && pixel(100).r == 0,
          "Provisional single-texture override changed");
    level.sectors[0].walls[1].type = 0x00ff;
    draw();
    check(pixel(0).r == pixel(0).g && pixel(0).g == pixel(0).b && pixel(0).r > 0,
          "Invalid material did not use flat-color fallback");
    level.sectors[0].walls[1].type = 0x0005;
    bank.textures[5] = {};
    draw();
    check(pixel(0).r == pixel(0).g && pixel(0).g == pixel(0).b,
          "Malformed material texture was sampled");
}
}

int main()
{
    try {
        testTextureSampling();
        testWallMaterials();
        game_lab::Level level{};
        game_lab::Sector sector{};
        sector.vertices = {{0, 0}, {100, 0}, {100, 100}, {0, 100},
                           {60, 0}, {60, 100}, {80, 0}, {100, 100}};
        sector.walls = {{0, 1, 0, 0}, {1, 2, 0x2000, 0},
                        {2, 3, 0, 0}, {3, 0, 0, 0}, {4, 5, 0x1001, 0}};
        sector.polygons.push_back({{0, 1, 2, 3}, 0, {}});
        level.sectors.push_back(sector);
        const Camera camera{50, 50, 0};
        const auto center = castWallRay(level, camera, 1, 0);
        check(std::abs(center.distance - 50) < 1e-8 && center.type == 0x2000,
              "Portal blocked visibility or nearest hit incorrect");
        check(std::abs(castWallRay(level, camera, 1, 0.4).distance - 50) < 1e-8,
              "Camera-plane perpendicular distance incorrect");
        check(std::abs(castWallRay(level, camera, -1, 0).distance - 50) < 1e-8,
              "Negative ray direction incorrect");
        level.sectors[0].walls.push_back({6, 7, 0x3000, 0});
        const auto diagonal = castWallRay(level, camera, 1, 0);
        check(std::abs(diagonal.distance - 40) < 1e-8 && diagonal.type == 0x3000,
              "Oblique segment intersection incorrect");
        const auto start = initialCamera(level);
        check(start.x > 1 && start.x < 99 && start.y > 1 && start.y < 99 &&
              std::abs(start.x - 60) > 1,
              "Camera not strictly inside polygon and clear of portal");
        for (const auto size : {SDL_Point{960, 600}, SDL_Point{600, 960}, SDL_Point{160, 100}}) {
            auto* surface = SDL_CreateSurface(size.x, size.y, SDL_PIXELFORMAT_RGBA32);
            check(surface != nullptr, "Surface creation failed");
            auto* renderer = SDL_CreateSoftwareRenderer(surface);
            check(renderer != nullptr, "Software renderer creation failed");
            check(drawFirstPerson(renderer, level, camera) && SDL_RenderPresent(renderer),
                  "Rendering failed");
            Uint8 r = 0, g = 0, b = 0, a = 0;
            check(SDL_ReadSurfacePixel(surface, size.x / 2, size.y / 2, &r, &g, &b, &a)
                  && r > g && b > g, "Nearest diagonal wall color incorrect");
            SDL_DestroyRenderer(renderer);
            SDL_DestroySurface(surface);
        }
        game_lab::Level empty{};
        check(!std::isfinite(castWallRay(empty, camera, 1, 0).distance),
              "Empty level should not produce a hit");
        bool rejected = false;
        try { initialCamera(empty); } catch (const std::runtime_error&) { rejected = true; }
        check(rejected, "Empty level should not produce a camera");
        std::cout << "Ray intersections, portals, camera placement and rendering passed.\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
