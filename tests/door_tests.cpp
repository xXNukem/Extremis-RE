#include "first_person_view.h"

#include <SDL3/SDL.h>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void check(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

game_lab::Level fixture()
{
    game_lab::Level level{};
    game_lab::Sector front{}, back{};
    front.vertices = {{100, 100}, {100, 0}, {200, 100}, {200, 0}};
    front.walls = {{0, 1, 0x1017, 0}, {2, 3, 1, 0}};
    back.vertices = {{100, 0}, {100, 100}};
    back.walls = {{0, 1, 0x1021, 0}};
    level.sectors = {front, back};
    level.connectionStates = {0x0400};
    return level;
}

void testRecognition()
{
    auto level = fixture();
    auto doors = doors::closedDoors(level);
    check(doors.size() == 1 && doors[0].stateIndex == 0 &&
          doors[0].originalState == 0x0400 && doors[0].faces[0].material == 23 &&
          doors[0].faces[1].material == 33, "Paired door/state/material recognition failed");
    check(level.sectors[0].walls[0].type == 0x1017, "Map wall was modified");
    level.sectors[0].walls[0].type = level.sectors[1].walls[0].type = 0x1000;
    check(doors::closedDoors(level).size() == 1,
          "Material zero was incorrectly treated as a passage");
    for (const auto state : {1, 30, 60, 90, 0x0100, 0x8000}) {
        level.connectionStates = {static_cast<std::uint16_t>(state)};
        check(doors::closedDoors(level).empty(), "Unsupported state guessed as closed door");
    }
    level.connectionStates.clear();
    check(doors::closedDoors(level).empty(), "Missing table created a door");
    level.connectionStates = {0};
    level.sectors[1].walls.clear();
    check(doors::closedDoors(level).empty(), "Unpaired passage created a door");
    level = fixture();
    level.sectors.push_back(level.sectors[1]);
    check(doors::closedDoors(level).empty(), "Ambiguous pair created a door");
}

void testVisibility()
{
    auto level = fixture();
    const auto doors = doors::closedDoors(level);
    const Camera front{50, 50, 0}, back{150, 50, 3.141592653589793};
    const auto a = castSceneRay(level, front, 1, 0, &doors);
    const auto b = castSceneRay(level, back, -1, 0, &doors);
    check(a.distance == 50 && a.type == 23 && a.segmentPosition == 0.5 &&
          b.distance == 50 && b.type == 33, "Door position or face selection failed");
    check(castWallRay(level, front, 1, 0).distance == 150,
          "Base portal traversal changed");
    check(castSceneRay(level, {50, 25, 0}, 1, 0, &doors).segmentPosition == 0.75 &&
          castSceneRay(level, {150, 25, 0}, -1, 0, &doors).segmentPosition == 0.25,
          "Door texture axis was mirrored");
    check(castSceneRay(level, {50, 0, 0}, 1, 0, &doors).segmentPosition == 1 &&
          castSceneRay(level, {50, 100, 0}, 1, 0, &doors).segmentPosition == 0,
          "Door endpoints were moved");
    level.sectors[0].vertices[2].x = level.sectors[0].vertices[3].x = 75;
    check(castSceneRay(level, front, 1, 0, &doors).distance == 25,
          "Door drew through foreground wall");
}

void testAnimationStates()
{
    auto list = doors::closedDoors(fixture());
    auto& door = list[0];
    check(door.state == doors::State::Closed && door.progress == 0,
          "Door did not start closed");
    doors::toggle(door);
    doors::update(door, 0.4);
    check(door.state == doors::State::Opening && std::abs(door.progress - 0.4) < 1e-12,
          "Opening progress incorrect");
    doors::toggle(door);
    check(door.state == doors::State::Closing && std::abs(door.progress - 0.4) < 1e-12,
          "Reversal reset progress");
    doors::update(door, 0.1);
    check(std::abs(door.progress - 0.3) < 1e-12, "Closing after reversal incorrect");
    doors::toggle(door);
    doors::update(door, 2.0);
    check(door.state == doors::State::Open && door.progress == 1,
          "Door did not finish opening");
    doors::toggle(door);
    doors::update(door, 2.0);
    check(door.state == doors::State::Closed && door.progress == 0,
          "Door did not finish closing");
}

void testIndependentTimeSteps()
{
    auto list = doors::closedDoors(fixture());
    list.push_back(list.front());
    doors::toggle(list[0]);
    doors::updateAll(list, 0.25);
    check(list[0].progress == 0.25 && list[1].progress == 0 &&
          list[1].state == doors::State::Closed, "Doors share animation state");
    doors::toggleAll(list);
    doors::updateAll(list, 0.1);
    check(std::abs(list[0].progress - 0.15) < 1e-12 &&
          std::abs(list[1].progress - 0.1) < 1e-12,
          "Independent directions or debug toggle failed");
    auto fine = doors::closedDoors(fixture())[0], coarse = fine;
    doors::toggle(fine);
    doors::toggle(coarse);
    for (int i = 0; i < 60; ++i) doors::update(fine, 1.0 / 60.0);
    doors::update(coarse, 1.0);
    check(fine.state == coarse.state && fine.progress == coarse.progress &&
          fine.state == doors::State::Open, "Opening depends on FPS");
    doors::toggle(fine);
    doors::toggle(coarse);
    for (int i = 0; i < 60; ++i) doors::update(fine, 1.0 / 60.0);
    doors::update(coarse, 1.0);
    check(fine.state == coarse.state && fine.progress == coarse.progress &&
          fine.state == doors::State::Closed, "Closing depends on FPS");
}

void testRetractedGeometry()
{
    const auto level = fixture();
    auto list = doors::closedDoors(level);
    auto& door = list[0];
    const auto original = door.faces;
    doors::toggle(door);
    for (int step = 1; step <= 3; ++step) {
        doors::update(door, 0.25);
        const auto front = doors::visibleFace(door, door.faces[0]);
        const auto back = doors::visibleFace(door, door.faces[1]);
        check(front && back && front->a.x == 100 && front->a.y == 100 - step * 25 &&
              front->b.y == 0 && back->a.y == step * 25 && back->b.y == 100 &&
              front->textureSpan == 1 - step * 0.25 && front->material == 23 &&
              back->material == 33, "Original per-face retraction or material changed");
        check(castSceneRay(level, {50, 90, 0}, 1, 0, &list).distance == 150,
              "Retracted opening still blocked by a door face");
    }
    doors::update(door, 0.25);
    check(!doors::visibleFace(door, door.faces[0]) &&
          !doors::visibleFace(door, door.faces[1]) &&
          castSceneRay(level, {50, 50, 0}, 1, 0, &list).distance == 150,
          "Fully open door has a visible surface");
    doors::toggle(door);
    doors::update(door, 1.0);
    const auto restored = doors::visibleFace(door, door.faces[0]);
    check(restored && restored->a.y == original[0].a.y &&
          restored->b.y == original[0].b.y && restored->textureSpan == 1 &&
          restored->material == original[0].material && door.originalState == 0x0400,
          "Closing did not restore original pose/material/state word");
}

void testTextureRetraction(FirstPersonRenderer& view, SDL_Renderer* renderer,
                           SDL_Surface* surface, const game_lab::Level& level)
{
    auto list = doors::closedDoors(level);
    wall_textures::Bank bank;
    bank.textures.resize(34, {1, 1, {{0, 200, 0}}});
    auto& texture = bank.textures[23];
    texture = {60, 74, {}};
    for (int y = 0; y < 74; ++y) {
        for (int x = 0; x < 60; ++x)
            texture.pixels.push_back({std::uint8_t(x * 4), std::uint8_t(y * 3), 17});
    }
    const auto drawPixel = [&]() {
        check(view.draw(level, {50, 25, 0}, nullptr, &bank, &list) &&
              SDL_RenderPresent(renderer), "Animated texture rendering failed");
        wall_textures::Rgb color{};
        Uint8 alpha;
        check(SDL_ReadSurfacePixel(surface, 50, 40, &color.r, &color.g, &color.b, &alpha)
              && alpha == 255, "Cannot read animated texture pixel");
        return color;
    };
    const auto initial = drawPixel();
    doors::toggleAll(list);
    const double shade = 0.2 + 0.8 / (1 + 50.0 / 700);
    for (int step = 1; step <= 3; ++step) {
        doors::updateAll(list, 0.25);
        const auto color = drawPixel();
        // Fixed world ray: the texture slides 15 columns per quarter stroke.
        const int column = 45 - 15 * step;
        check(color.r == Uint8(column * 4 * shade) &&
              color.g == Uint8(37 * 3 * shade) && color.b == Uint8(17 * shade),
              "Moving door stretched/mirrored texture or changed vertical extent");
    }
    doors::updateAll(list, 0.25);
    const auto open = drawPixel();
    check(open.r == 0 && open.g > 0 && open.b == 0,
          "Fully open door did not reveal the wall behind it");
    doors::toggleAll(list);
    doors::updateAll(list, 1.0);
    const auto restored = drawPixel();
    check(restored.r == initial.r && restored.g == initial.g && restored.b == initial.b,
          "Closing did not restore the original texture sampling");
}

void testDrawing()
{
    const auto level = fixture();
    const auto doors = doors::closedDoors(level);
    wall_textures::Bank bank;
    bank.textures.resize(34, {1, 1, {{0, 0, 0}}});
    bank.textures[23].pixels[0] = {255, 0, 0};
    bank.textures[33].pixels[0] = {0, 0, 255};
    std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface(
        SDL_CreateSurface(101, 81, SDL_PIXELFORMAT_RGBA32), SDL_DestroySurface);
    check(bool(surface), "Door test surface failed");
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer(
        SDL_CreateSoftwareRenderer(surface.get()), SDL_DestroyRenderer);
    check(bool(renderer), "Door test renderer failed");
    FirstPersonRenderer view(renderer.get());
    for (const auto camera : {Camera{50, 50, 0}, Camera{150, 50, 3.141592653589793}}) {
        check(view.draw(level, camera, nullptr, &bank, &doors) &&
              SDL_RenderPresent(renderer.get()), "Door rendering failed");
        Uint8 r, g, b, a;
        check(SDL_ReadSurfacePixel(surface.get(), 50, 40, &r, &g, &b, &a) &&
              g == 0 && a == 255 && (camera.x < 100 ? r > 0 && b == 0 : b > 0 && r == 0),
              "Door DAL material was not rendered on the correct face");
        check(view.draw(level, camera, nullptr, nullptr, &doors), "Flat door fallback failed");
    }
    testTextureRetraction(view, renderer.get(), surface.get(), level);
}
} // namespace

int main()
{
    try {
        testRecognition();
        testVisibility();
        testAnimationStates();
        testIndependentTimeSteps();
        testRetractedGeometry();
        testDrawing();
        std::cout << "Closed door recognition, faces, occlusion and rendering passed.\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
