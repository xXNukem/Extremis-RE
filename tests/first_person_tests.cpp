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

void testTextureSampling(double height)
{
    game_lab::Level level{};
    game_lab::Sector sector{};
    sector.vertices = {{100, 0}, {100, 100}};
    sector.walls = {{0, 1, 0, 0}};
    level.sectors.push_back(sector);
    Camera camera{50, 50, 0};
    camera.postureHeight=height;
    wall_textures::Texture texture{60, 74, {}};
    for (int y = 0; y < texture.height; ++y) {
        for (int x = 0; x < texture.width; ++x) {
            texture.pixels.push_back(x < 30 && y != 37
                ? wall_textures::Rgb{std::uint8_t(y * 3), std::uint8_t(x * 8), 17}
                : wall_textures::Rgb{0, 0, 0});
        }
    }
    check(castWallRay(level, camera, 1, 1).segmentPosition == 1 &&
          castWallRay(level, camera, 1, -1).segmentPosition == 0,
          "Endpoint intersection fractions incorrect");
    for (const auto type : {0x0003, 0x2003, 0x2016, 0x2020, 0x202a, 0x2034, 0x3003}) {
        level.sectors[0].walls[0].type = type;
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
            const double top = size.y * 0.5 - (256-camera.viewHeight()) * focal / 50;
            const double wallPixels = 256 * focal / 50;
            const double shade = 0.2 + 0.8 / (1 + 50.0 / 700);
            for (const int x : {0, size.x / 2, size.x - 1}) {
                const double offset = (2 * (x + 0.5) / size.x - 1) *
                                      std::tan(70.0 * 3.14159265358979323846 / 360);
                const double u = (50 - 50 * offset) / 100;
                const int columns = (type & 0xf000) == 0x2000 ? 30 : 60;
                const int texX = int(u * columns);
                for (const int y : {0, size.y / 2, size.y - 1}) {
                    const int texY = int((y + 0.5 - top) / wallPixels * texture.height);
                    Uint8 r = 0, g = 0, b = 0, a = 0;
                    check(SDL_ReadSurfacePixel(surface.get(), x, y, &r, &g, &b, &a) &&
                          r == Uint8((texX < 30 && texY != 37 ? texY * 3 : 0) * shade) &&
                          g == Uint8((texX < 30 && texY != 37 ? texX * 8 : 0) * shade) &&
                          b == Uint8((texX < 30 && texY != 37 ? 17 : 0) * shade) && a == 255,
                          "Frame U range, opaque black, axis direction or clipped sampling mismatch");
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
}

void testWallMaterials()
{
    check(wallTextureColumnCount(0x2003, 60) == 30 &&
          wallTextureColumnCount(0x2034, 60) == 30 &&
          wallTextureColumnCount(0x0003, 60) == 60 &&
          wallTextureColumnCount(0x3003, 60) == 60 &&
          wallTextureColumnCount(0x2003, 1) == 1,
          "Frame column interval or other material families changed");
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

void testSharedDiagonal()
{
    // Level 2, sectors 4/6: same boundary, reversed 133/133/134 cuts.
    game_lab::Sector a{}, b{};
    a.vertices = {{4000,2000},{3867,1867},{3734,1734},{3600,1600}};
    a.walls = {{0,1,0x1a,0},{1,2,0x1c,0},{2,3,0x1d,0}};
    b.vertices = {{3600,1600},{3733,1733},{3866,1866},{4000,2000}};
    b.walls = {{0,1,0x24,0},{1,2,0x26,0},{2,3,0x27,0}};
    for (bool reverseOrder : {false,true}) {
        game_lab::Level shared{};
        shared.sectors = reverseOrder ? std::vector<game_lab::Sector>{b,a}
                                      : std::vector<game_lab::Sector>{a,b};
        for (int side : {-1,1}) {
            game_lab::Level facing{};
            facing.sectors = {side == -1 ? a : b};
            for (int i=1; i<400; ++i) {
                const double x=3600+i+0.123, y=1600+i+0.123;
                const Camera camera{x+side*50,y-side*50,0};
                const auto expected=castWallRay(facing,camera,-side,side);
                const auto actual=castWallRay(shared,camera,-side,side);
                check(actual.type==expected.type &&
                      std::abs(actual.distance-expected.distance)<1e-9 &&
                      std::abs(actual.segmentPosition-expected.segmentPosition)<1e-9,
                      "Shared diagonal selected opposite face or changed texture U");
            }
        }
    }
}
}

int main()
{
    try {
        testTextureSampling(165);
        testTextureSampling(100);
        testWallMaterials();
        testSharedDiagonal();
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
        auto originalStartLevel=level;
        originalStartLevel.playerStart=game_lab::PlayerStart{0,30,32,180};
        const auto dosStart=initialCamera(originalStartLevel);
        check(dosStart.x==30 && dosStart.y==32 && std::abs(dosStart.angle-1.5707963267948966)<1e-9,
              "Camera ignored original start or changed angle convention");
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
