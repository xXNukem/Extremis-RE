#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "game_lab.h"
#include "map_view.h"
#include "first_person_view.h"
#include "texture_preview.h"
#include "player_input.h"
#include "debug_menu.h"
#include "plane_appearance.h"
#include "sprite_animation.h"
#include "decorative_animation.h"
#include "objet_collision.h"
#include "game_audio.h"
#include "survival_session.h"
#include "item_session.h"
#include "map_session.h"
#include "enemy_system.h"
#include "startup_credits.h"

#include <exception>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string_view>

int main(int argc, char* argv[])
{
#ifdef EXTREMIS_DEBUG_MENU
    const bool vitalsDebug = argc == 3 && std::string_view(argv[2]) == "--vitals-debug";
#else
    const bool vitalsDebug = false;
#endif
    if (argc > 3 || (argc == 3 && !vitalsDebug)) {
        std::cerr << "Usage: " << argv[0] << " [path/to/GAME.LAB"
#ifdef EXTREMIS_DEBUG_MENU
                  << " [--vitals-debug]"
#endif
                  << "]\n";
        return 1;
    }
    std::optional<game_lab::Level1> data;
    std::optional<Camera> camera;
    TexturePreview texturePreview;
    PlaneAppearance planeAppearance;
    std::optional<sprites::Scene> spriteScene;
    sprites::Animation spriteAnimation;
    sprites::DecorativeAnimation decorativeAnimation;
    std::optional<enemies::System> enemySystem;
    std::optional<hud::Renderer> originalHud;
    std::optional<interaction::Renderer> interactionRenderer;
    SurvivalSession survival(vitalsDebug);
    ItemSession items;
    original_map::Session mapSession;
    std::optional<original_map::Renderer> originalMap;
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
                originalMap.emplace(original_map::load(std::filesystem::path(argv[1]).parent_path()/"DATA"));
            } catch (const std::exception& error) {
                std::cerr << "Original map unavailable: " << error.what() << '\n';
            }
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
                      << "M / physical semicolon: toggle original map after plan transfer; Space: close map.\n"
                      << "Left/Right: rotate in 3D.\n"
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
    const auto credits=showStartupCredits(window,renderer,
        argc>=2 ? std::filesystem::path(argv[1]).parent_path() / "DATA"
                : std::filesystem::path{});
    if (credits==StartupCreditsResult::Error) {
        SDL_Log("Startup credits failed: %s",SDL_GetError());
        exitCode=1;
    }
    if (credits==StartupCreditsResult::Continue) {
        FirstPersonRenderer firstPerson(renderer);
        DoorControl doorControl(collisionWorld, levelDoors);
        PlayerInput playerInput(window, doorControl);
        GameAudio gameAudio;
        if (data) gameAudio.load(std::filesystem::path(argv[1]).parent_path() / "DATA");
#ifdef EXTREMIS_DEBUG_MENU
        DebugMenu debugMenu;
        if (data) {
            try { debugMenu.load(argv[1]); }
            catch (const std::exception& error) { SDL_Log("Debug catalog: %s",error.what()); }
        }
#endif
        bool running = true;
        bool showPolygons = false;
        bool show3D = camera.has_value();
        Uint64 previousTicks = SDL_GetTicks();
        while (running) {
            auto* activeEnemies=enemySystem?&*enemySystem:nullptr;
            bool debugPaused=false;
#ifdef EXTREMIS_DEBUG_MENU
            if (!debugMenu.enemiesEnabled()) activeEnemies=nullptr;
            debugPaused=debugMenu.open();
#endif
            mapSession.beginFrame(data?data->level.zoneIndex:0);
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
#ifdef EXTREMIS_DEBUG_MENU
                if (data && event.type!=SDL_EVENT_QUIT && event.type!=SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                    !((event.type==SDL_EVENT_KEY_DOWN || event.type==SDL_EVENT_KEY_UP) && event.key.windowID!=SDL_GetWindowID(window)) &&
                    debugMenu.event(event,survival.inventory(),data->level.zoneIndex)) {
                    items.clearInput(); playerInput.handleEvent(event,false); playerInput.invalidateContext();
                    activeEnemies=debugMenu.enemiesEnabled() && enemySystem?&*enemySystem:nullptr;
                    continue;
                }
#endif
                const bool wasMap=mapSession.modal();
                const bool mapKey=event.type==SDL_EVENT_KEY_DOWN &&
                    (event.key.scancode==SDL_SCANCODE_M || event.key.scancode==SDL_SCANCODE_SEMICOLON);
                const bool mapBlocked=mapKey && activeEnemies && camera && data &&
                    activeEnemies->blocksInterface(data->level,*camera,levelDoors);
                const bool mapEvent=mapSession.handleEvent(event,SDL_GetWindowID(window),
                    camera?&*camera:nullptr,show3D && !items.busy() && SDL_GetKeyboardFocus()==window,
                    items.planTransferred(),mapBlocked,originalMap.has_value() && originalHud.has_value(),survival);
                if (wasMap!=mapSession.modal()) { items.clearInput(); playerInput.invalidateContext(); }
                // Releases must clear held UI keys even while the map owns input.
                const bool itemEvent=(!mapEvent || event.type==SDL_EVENT_KEY_UP) &&
                    items.handleEvent(event,SDL_GetWindowID(window));
                const bool modalEvent=mapEvent || itemEvent;
                // Observe releases even if the UI owns the event, but never arm UI clicks.
                playerInput.handleEvent(event,!modalEvent && !items.modal() && !mapSession.blocksSimulation() && show3D && survival.alive());
                if (modalEvent) continue;
                if ((event.type==SDL_EVENT_KEY_DOWN || event.type==SDL_EVENT_KEY_UP) &&
                    event.key.windowID!=SDL_GetWindowID(window)) continue;
                survival.handleEvent(event, SDL_GetWindowID(window));
                planeAppearance.handleEvent(event, SDL_GetWindowID(window));
#ifdef EXTREMIS_DEBUG_MENU
                if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat &&
                    event.key.windowID==SDL_GetWindowID(window) &&
                    (event.key.scancode == SDL_SCANCODE_PAGEDOWN || event.key.scancode == SDL_SCANCODE_PAGEUP)) {
                    texturePreview.changeIndex(event.key.scancode == SDL_SCANCODE_PAGEUP ? 1 : -1);
                    SDL_SetWindowTitle(window, survival.windowTitle(texturePreview.windowTitle()).c_str());
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
#endif
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
#ifdef EXTREMIS_DEBUG_MENU
            debugPaused=debugMenu.open();
            activeEnemies=debugMenu.enemiesEnabled() && enemySystem?&*enemySystem:nullptr;
#endif
            const bool inputActive = camera && show3D && !debugPaused && SDL_GetKeyboardFocus() == window;
            // Resolve context changes before polling continuous WASD/Shift state.
            items.update(camera ? &*camera : nullptr,inputActive && !mapSession.blocksSimulation(),
                         doorControl.movementWorld(),survival,gameAudio,&doorControl,elapsedSeconds);
                auto travel=items.takeTravel();
#ifdef EXTREMIS_DEBUG_MENU
                if (auto requested=debugMenu.takeTravel()) {
                    travel=requested;
                    // Dismiss an interrupted interface without resetting collected items.
                    items.setAvailable(false);
                    items.setAvailable(interactionRenderer.has_value() && originalHud.has_value());
                    mapSession=original_map::Session{};
                }
#endif
                if (travel) {

                    // An elevator travelling within the same level must not reload the world.
                    if (data && travel->zone == data->level.zoneIndex) {
                        auto arrivalLevel = data->level;
                        arrivalLevel.playerStart = travel->arrival;

                        camera = initialCamera(arrivalLevel);
                        playerInput.invalidateContext();

                        SDL_Log("Elevator: same-level transfer to sector %u",
                                travel->arrival.sector);

                        previousTicks = SDL_GetTicks();
                        continue;
                    }

                    try {
                    // Stage destination resources before replacing the active world.
                    auto next=game_lab::loadLevel(argv[1],travel->zone);
                    next.level.playerStart=travel->arrival;
                    const auto nextCamera=initialCamera(next.level);
                    const auto directory=std::filesystem::path(argv[1]).parent_path()/"DATA";
                    auto nextSprites=sprites::loadScene(next.level,directory);
                    enemies::System nextEnemies; nextEnemies.load(next.level,directory);
                    auto nextHud=hud::Renderer(hud::load(directory,next.level.palBank));
                    player::WeaponFire nextWeapon; nextWeapon.loadGraphics(directory,next.level.palBank);
                    TexturePreview nextTextures;
                    nextTextures.load(directory,next.level.dalleBank,next.level.palBank);
                    PlaneAppearance nextPlanes;
                    nextPlanes.load(directory,next.level.dalleBank,next.level.palBank);
                    if (!nextTextures.materials())
                        throw std::runtime_error("Missing destination wall textures");
                    auto nextWorld=collision::staticWorld(next.level);
                    auto nextDoors=doors::closedDoors(next.level);
                    items.load(next.level); // Keeps transferred plans; clears level-local terminals.
                    data=std::move(next); camera=nextCamera;
                    collisionWorld=std::move(nextWorld); levelDoors=std::move(nextDoors);
                    doorControl.resetLevel();
                    spriteScene=std::move(nextSprites); spriteAnimation=sprites::Animation{};
                    decorativeAnimation=sprites::DecorativeAnimation{};
                    enemySystem=std::move(nextEnemies); originalHud=std::move(nextHud);
                    weaponFire=std::move(nextWeapon); weaponGraphicsReady=true;
                    texturePreview=std::move(nextTextures); planeAppearance=std::move(nextPlanes);
                    mapSession=original_map::Session{};
                    mapSession.beginFrame(data->level.zoneIndex);
                    survival.clearMessages(); // Vitals, inventory and battery retain their values.
                    playerInput.invalidateContext();
                    SDL_SetWindowTitle(window,survival.windowTitle(texturePreview.windowTitle()).c_str());
                    SDL_Log("Elevator: level %u, sector %u, arrival %.0f,%.0f; %zu sectors, %zu sprites, %zu enemies, %zu doors",
                        travel->zone+1,travel->arrival.sector,camera->x,camera->y,
                        data->level.sectors.size(),spriteScene->objects.size(),
                        enemySystem->records().size(),levelDoors.size());
                } catch (const std::exception& error) {
                    SDL_Log("Elevator destination load failed: %s",error.what());
                }
                previousTicks=SDL_GetTicks();
                continue; // No simulation with elapsed time or input from the old world.
            }
            const bool modalBlocked = debugPaused || items.modal() || mapSession.blocksSimulation();
            const bool simulationActive = inputActive && !modalBlocked;
            mapSession.update(debugPaused?0:elapsedSeconds,SDL_GetKeyboardFocus()==window);
            texturePreview.update(modalBlocked?0:elapsedSeconds);
            if (spriteScene && !modalBlocked) spriteAnimation.update(*spriteScene, elapsedSeconds);
            if (spriteScene && simulationActive) decorativeAnimation.update(*spriteScene, elapsedSeconds);
            const Camera beforeMovement=camera.value_or(Camera{});
            playerInput.setAudioEnabled(gameAudio.enabled());
            playerInput.setObjectConstraint([&](const Camera& before, Camera& candidate) {
                if (!data) return;
                const auto location = collision::locate(data->level,{before.x,before.y});
                if (!location) return;
                const auto corrected = collision::constrainObjets(data->level,location->sector,
                    {before.x,before.y},{candidate.x,candidate.y},spriteScene ? &*spriteScene : nullptr);
                candidate.x = corrected.x; candidate.y = corrected.y;
            });
            if (!playerInput.update(camera ? &*camera : nullptr, show3D && survival.alive() && !modalBlocked,
                                    modalBlocked?0:elapsedSeconds,activeEnemies && data)) {
                SDL_Log("Player input failed: %s", SDL_GetError());
                exitCode = 1;
                break;
            }

            if (activeEnemies && camera && data)
                activeEnemies->update(data->level,*camera,doorControl.movementWorld(),survival.vitals(),
                    elapsedSeconds,simulationActive && survival.alive(),
                    [&](double seconds) {
                        playerInput.move(*camera,seconds,activeEnemies->reacting(),
                            [&](const Camera& before, Camera& after) {
                                activeEnemies->constrainPlayer(data->level,before,after,doorControl.movementWorld());
                            });
                    },
                    [&]() {
                        const auto before=*camera;
                        playerInput.recoil(*camera);
                        activeEnemies->constrainPlayer(data->level,before,*camera,doorControl.movementWorld());
                    },levelDoors,spriteScene ? &*spriteScene : nullptr);
            const double effectiveMovement=camera ? std::hypot(camera->x-beforeMovement.x,camera->y-beforeMovement.y) : 0;
            if (activeEnemies) for (const auto sound:activeEnemies->takeSounds()) gameAudio.enemyEffect(sound);
            if (survival.update(elapsedSeconds,simulationActive,gameAudio,playerInput.running() && effectiveMovement>0))
                SDL_SetWindowTitle(window,survival.windowTitle(texturePreview.windowTitle()).c_str());
            const auto fired=weaponFire.update(camera?*camera:Camera{},doorControl.movementWorld(),
                survival.inventory(),playerInput.takeFireTrigger(),
                simulationActive && survival.alive() && playerInput.fireReady() && originalHud && weaponGraphicsReady,
                elapsedSeconds,activeEnemies);
            if (fired==player::FireResult::Fired) gameAudio.fireWeapon(survival.inventory().weapon());
            else if (fired==player::FireResult::Empty || fired==player::FireResult::Full) gameAudio.emptyWeapon();
            if (activeEnemies) for (const auto sound:activeEnemies->takeSounds()) gameAudio.enemyEffect(sound);
            auto hudState=survival.hudState();
            if (activeEnemies && camera && data) {
                hudState.slimePhase=activeEnemies->effects().slimePhase();
                hudState.detector=detector::scan(data->level,activeEnemies->records(),*camera,
                    activeEnemies->simulationTicks(),survival.inventory().detectorEnabled());
            }
            hudState.firing=weaponFire.firing(); hudState.barrelPhase=weaponFire.barrelPhase();
            gameAudio.update(levelDoors, playerInput.footsteps());
            if (!SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) ||
                !SDL_RenderClear(renderer) ||
                (data && !(show3D && camera
                    ? firstPerson.draw(data->level, *camera, texturePreview.selectedTexture(),
                                       texturePreview.materials(), &levelDoors, planeAppearance.materials(),
                                       spriteScene ? &*spriteScene : nullptr,
                                       originalHud ? &*originalHud : nullptr, hudState,
                                       interactionRenderer ? &*interactionRenderer : nullptr,items.interfaceState(),
                                       weaponGraphicsReady?&weaponFire.scene():nullptr,
                                       activeEnemies?&activeEnemies->scene():nullptr,
                                       activeEnemies?&activeEnemies->effects().objects():nullptr,
                                       originalMap?&*originalMap:nullptr,mapSession.state())
                    : drawMap(renderer, data->level, showPolygons, camera ? &*camera : nullptr))) ||
#ifdef EXTREMIS_DEBUG_MENU
                !debugMenu.draw(renderer,data?data->level.zoneIndex:0) ||
#endif
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
