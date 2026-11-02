#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "game_lab.h"
#include "map_view.h"
#include "first_person_view.h"
#include "texture_preview.h"

#include <exception>
#include <iomanip>
#include <iostream>
#include <optional>
#include <algorithm>
#include <cmath>

int main(int argc, char* argv[])
{
    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [path/to/GAME.LAB]\n";
        return 1;
    }
    std::optional<game_lab::Level1> data;
    std::optional<Camera> camera;
    TexturePreview texturePreview;
    if (argc == 2) {
        try {
            data = game_lab::loadLevel1(argv[1]);
            const auto& level = data->level;
            texturePreview.load(std::filesystem::path(argv[1]).parent_path() / "DATA",
                                level.dalleBank, level.palBank);
            try {
                camera = initialCamera(level);
                std::cout << "Initial camera: x=" << camera->x << " y=" << camera->y
                          << " z=128 angle=" << camera->angle
                          << " rad (0 = +X; positive = towards +Y)\n";
            } catch (const std::exception& error) {
                std::cerr << "3D view unavailable: " << error.what() << '\n';
            }
            std::size_t walls = 0, entities = 0, polygons = 0, connections = 0;
            for (const auto& sector : level.sectors) {
                walls += sector.walls.size();
                entities += sector.entitiesA.size();
                polygons += sector.polygons.size();
                for (const auto& polygon : sector.polygons) {
                    connections += polygon.connections.size();
                }
            }
            std::cout << "GAME.LAB: " << data->header.levelCount << " levels\n"
                      << "Level 1 banks: PAL=" << level.palBank
                      << " DALLE=" << level.dalleBank << " OBJET=" << level.objetBank
                      << " MONS=" << level.monsBank << '\n'
                      << "Sectors: " << level.sectors.size() << " | Walls: " << walls
                      << " | Entities A: " << entities << '\n'
                      << "Polygons B: " << polygons << " | Connections: " << connections << '\n'
                      << "Press P to toggle polygon contours.\n"
                      << "TAB: 2D/3D view; Left/Right: rotate in 3D.\n"
                      << "Entity sectors use zero-based indices.\n";
            for (std::size_t i = 0; i < level.sectors.size(); ++i) {
                const auto& sector = level.sectors[i];
                for (const auto& entity : sector.entitiesA) {
                    const auto& vertex = sector.vertices[entity.vertex];
                    std::cout << "  sector=" << i << " type=0x" << std::hex
                              << std::setw(4) << std::setfill('0') << entity.type
                              << std::dec << std::setfill(' ')
                              << " x=" << (std::uint32_t(sector.gx) + vertex.x)
                              << " y=" << (std::uint32_t(sector.gy) + vertex.y)
                              << " param1=" << entity.param1
                              << " param2=" << entity.param2 << '\n';
                }
            }
            std::cout << std::flush;
        } catch (const std::exception& error) {
            std::cerr << "GAME.LAB load failed: " << error.what() << '\n';
            return 1;
        }
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL initialization failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(texturePreview.windowTitle().c_str(), 960, 600,
                                           SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        SDL_Log("Renderer creation failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    int exitCode = 0;
    {
        FirstPersonRenderer firstPerson(renderer);
        bool running = true;
        bool showPolygons = false;
        bool show3D = false;
        Uint64 previousTicks = SDL_GetTicks();
        while (running) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat &&
                    (event.key.key == SDLK_N || event.key.key == SDLK_M)) {
                    texturePreview.changeIndex(event.key.key == SDLK_M ? 1 : -1);
                    SDL_SetWindowTitle(window, texturePreview.windowTitle().c_str());
                }
                if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_TAB &&
                    !event.key.repeat && camera) {
                    show3D = !show3D;
                    SDL_Log("View: %s", show3D ? "3D" : "2D");
                }
                if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_P &&
                    !event.key.repeat) {
                    showPolygons = !showPolygons;
                    SDL_Log("Polygon contours: %s", showPolygons ? "on" : "off");
                }
                if (event.type == SDL_EVENT_QUIT ||
                    event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED ||
                    (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
                    running = false;
                }
            }

            if (!running) {
                break;
            }

            const Uint64 ticks = SDL_GetTicks();
            const double dt = std::min(double(ticks - previousTicks) / 1000.0, 0.1);
            previousTicks = ticks;
            if (show3D && camera && (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS)) {
                const bool* keys = SDL_GetKeyboardState(nullptr);
                const double turn = double(keys[SDL_SCANCODE_LEFT]) -
                                    double(keys[SDL_SCANCODE_RIGHT]);
                camera->angle = std::remainder(camera->angle + turn * 1.5 * dt,
                                               2 * 3.14159265358979323846);
            }

            if (!SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) ||
                !SDL_RenderClear(renderer) ||
                (data && !(show3D && camera
                    ? firstPerson.draw(data->level, *camera, texturePreview.selectedTexture(),
                                       texturePreview.materials())
                    : drawMap(renderer, data->level, showPolygons))) ||
                !SDL_RenderPresent(renderer)) {
                SDL_Log("Rendering failed: %s", SDL_GetError());
                exitCode = 1;
                break;
            }

            SDL_Delay(16);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return exitCode;
}
