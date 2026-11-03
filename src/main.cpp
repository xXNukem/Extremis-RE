#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "game_lab.h"
#include "map_view.h"
#include "first_person_view.h"
#include "texture_preview.h"
#include "player_input.h"
#include "player_debug.h"
#include "plane_appearance.h"
#include "sprite_animation.h"
#include "game_audio.h"
#include "survival_session.h"
#include "item_session.h"
#include "enemy_system.h"

#include <exception>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string_view>

int main(int argc, char* argv[])
{
    const bool vitalsDebug = argc == 3 && std::string_view(argv[2]) == "--vitals-debug";
    if (argc > 3 || (argc == 3 && !vitalsDebug)) {
        std::cerr << "Usage: " << argv[0] << " [path/to/GAME.LAB [--vitals-debug]]\n";
        return 1;
    }
    std::optional<game_lab::Level1> data;
    std::optional<Camera> camera;
    TexturePreview texturePreview;
    PlaneAppearance planeAppearance;
    std::optional<sprites::Scene> spriteScene;
    sprites::Animation spriteAnimation;
    std::optional<enemies::System> enemySystem;
    std::optional<hud::Renderer> originalHud;
    std::optional<interaction::Renderer> interactionRenderer;
    SurvivalSession survival(vitalsDebug);
    ItemSession items;
    player::WeaponFire weaponFire;
    bool weaponGraphicsReady=false;
    std::vector<doors::Door> levelDoors;
    collision::World collisionWorld;
    if (argc >= 2) {
        try {
            data = game_lab::loadLevel1(argv[1]);
            const auto& level = data->level;
            items.load(level);
            try {
                interactionRenderer.emplace(interaction::load(std::filesystem::path(argv[1]).parent_path()/"DATA"));
            } catch (const std::exception& error) {
                items.setAvailable(false);
                std::cerr << "Original interaction resources unavailable: " << error.what() << '\n';
            }
            try {
                originalHud.emplace(hud::load(std::filesystem::path(argv[1]).parent_path() / "DATA",
                                              level.palBank));
                std::cout << "Player vitals: original initial health=1000, oxygen=288000; "
                             "160 nominal ticks/second during active 3D simulation.\n";
            } catch (const std::exception& error) {
                std::cerr << "HUD unavailable: " << error.what() << '\n';
            }
            items.setAvailable(interactionRenderer.has_value() && originalHud.has_value());
            try {
                weaponFire.loadGraphics(std::filesystem::path(argv[1]).parent_path()/"DATA",level.palBank);
                weaponGraphicsReady=true;
            } catch (const std::exception& error) {
                std::cerr<<"Weapon graphics unavailable; firing disabled: "<<error.what()<<'\n';
            }
            collisionWorld = collision::staticWorld(level);
            levelDoors = doors::closedDoors(level);
            std::cout << "Verified doors (initially closed): " << levelDoors.size() << '\n';
            texturePreview.load(std::filesystem::path(argv[1]).parent_path() / "DATA",
                                level.dalleBank, level.palBank);
            planeAppearance.load(std::filesystem::path(argv[1]).parent_path() / "DATA",
                                 level.dalleBank, level.palBank);
            try {
                spriteScene = sprites::loadScene(level, std::filesystem::path(argv[1]).parent_path() / "DATA");
                std::cout << "OBJET sprites: " << spriteScene->bank.images.size()
                          << " images, " << spriteScene->objects.size() << " instances, "
                          << spriteScene->skipped << " unsupported records.\n";
            } catch (const std::exception& error) {
                std::cerr << "Sprites unavailable: " << error.what() << '\n';
            }
            try {
                enemies::System enemies;
                enemies.load(level, std::filesystem::path(argv[1]).parent_path() / "DATA");
                enemySystem = std::move(enemies);
                std::cout << "MONS1: " << enemySystem->records().size() << " original enemies.\n";
            } catch (const std::exception& error) {
                std::cerr << "Enemies unavailable: " << error.what() << '\n';
            }
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
                      << "TAB: 2D/3D view; Left/Right: rotate in 3D; O: toggle doors (debug).\n"
                      << "WASD: move/navigate; Shift: run; mouse: look; left button: fire equipped weapon.\n"
                      << "E: interact/confirm; I: toggle inventory; Z: heal; X: oxygen; Escape: close interface.\n"
                      << "F: toggle floor and ceiling textures together.\n"
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

    SDL_Window* window = SDL_CreateWindow(survival.windowTitle(texturePreview.windowTitle()).c_str(), 960, 600,
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
        DoorControl doorControl(collisionWorld, levelDoors);
        PlayerInput playerInput(window, doorControl);
        GameAudio gameAudio;
        if (data) gameAudio.load(std::filesystem::path(argv[1]).parent_path() / "DATA");
        bool running = true;
        bool showPolygons = false;
        bool show3D = false;
        Uint64 previousTicks = SDL_GetTicks();
        while (running) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                const bool modalEvent=items.handleEvent(event,SDL_GetWindowID(window));
                // Observe releases even if the UI owns the event, but never arm UI clicks.
                playerInput.handleEvent(event,!modalEvent && !items.modal() && show3D && survival.alive());
                if (modalEvent) continue;
                survival.handleEvent(event, SDL_GetWindowID(window));
                planeAppearance.handleEvent(event, SDL_GetWindowID(window));
                if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat &&
                    (event.key.key == SDLK_N || event.key.key == SDLK_M)) {
                    texturePreview.changeIndex(event.key.key == SDLK_M ? 1 : -1);
                    SDL_SetWindowTitle(window, survival.windowTitle(texturePreview.windowTitle()).c_str());
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
                if (survival.alive() && event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_O &&
                    !event.key.repeat) {
                    doors::toggleAll(levelDoors);
                }
                if (event.type == SDL_EVENT_QUIT ||
                    event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED ||
                    (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE && !event.key.repeat)) {
                    running = false;
                }
            }

            if (!running) {
                break;
            }

            const Uint64 ticks = SDL_GetTicks();
            const double elapsedSeconds = double(ticks - previousTicks) / 1000.0;
            previousTicks = ticks;
            const bool inputActive = camera && show3D && SDL_GetKeyboardFocus() == window;
            // Resolve context changes before polling continuous WASD/Shift state.
            items.update(camera ? &*camera : nullptr,inputActive,
                         doorControl.movementWorld(),survival,gameAudio,&doorControl);
            const bool simulationActive = inputActive && !items.modal();
            texturePreview.update(items.modal()?0:elapsedSeconds);
            if (spriteScene && !items.modal()) spriteAnimation.update(*spriteScene, elapsedSeconds);
            const Camera beforeMovement=camera.value_or(Camera{});
            if (!playerInput.update(camera ? &*camera : nullptr, show3D && survival.alive() && !items.modal(),
                                    items.modal()?0:elapsedSeconds)) {
                SDL_Log("Player input failed: %s", SDL_GetError());
                exitCode = 1;
                break;
            }

            if (enemySystem && camera && data && simulationActive && survival.alive())
                enemySystem->constrainPlayer(data->level,beforeMovement,*camera,doorControl.movementWorld());
            const double effectiveMovement=camera ? std::hypot(camera->x-beforeMovement.x,camera->y-beforeMovement.y) : 0;

            if (enemySystem && camera && data)
                enemySystem->update(data->level,*camera,doorControl.movementWorld(),survival.vitals(),
                                    elapsedSeconds,simulationActive && survival.alive());
            if (enemySystem) for (const auto sound:enemySystem->takeSounds()) gameAudio.enemyEffect(sound);
            if (survival.update(elapsedSeconds,simulationActive,gameAudio,playerInput.running() && effectiveMovement>0))
                SDL_SetWindowTitle(window,survival.windowTitle(texturePreview.windowTitle()).c_str());
            const auto fired=weaponFire.update(camera?*camera:Camera{},doorControl.movementWorld(),
                survival.inventory(),playerInput.takeFireTrigger(),
                simulationActive && survival.alive() && playerInput.fireReady() && originalHud && weaponGraphicsReady,
                elapsedSeconds,enemySystem?&*enemySystem:nullptr);
            if (fired==player::FireResult::Fired) gameAudio.fireWeapon(survival.inventory().weapon());
            else if (fired==player::FireResult::Empty || fired==player::FireResult::Full) gameAudio.emptyWeapon();
            if (enemySystem) for (const auto sound:enemySystem->takeSounds()) gameAudio.enemyEffect(sound);
            auto hudState=survival.hudState();
            if (enemySystem && camera && data) {
                hudState.slimePhase=enemySystem->effects().slimePhase();
                hudState.detector=detector::scan(data->level,enemySystem->records(),*camera,
                    enemySystem->simulationTicks(),survival.inventory().detectorEnabled());
            }
            hudState.firing=weaponFire.firing(); hudState.barrelPhase=weaponFire.barrelPhase();
            gameAudio.update(levelDoors, effectiveMovement);
            if (!SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) ||
                !SDL_RenderClear(renderer) ||
                (data && !(show3D && camera
                    ? firstPerson.draw(data->level, *camera, texturePreview.selectedTexture(),
                                       texturePreview.materials(), &levelDoors, planeAppearance.materials(),
                                       spriteScene ? &*spriteScene : nullptr,
                                       originalHud ? &*originalHud : nullptr, hudState,
                                       interactionRenderer ? &*interactionRenderer : nullptr,items.interfaceState(),
                                       weaponGraphicsReady?&weaponFire.scene():nullptr,
                                       enemySystem?&enemySystem->scene():nullptr,
                                       enemySystem?&enemySystem->effects().objects():nullptr)
                    : drawMap(renderer, data->level, showPolygons, camera ? &*camera : nullptr))) ||
                (camera && (!show3D || !originalHud) && !drawPlayerDebug(renderer, *camera)) ||
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
