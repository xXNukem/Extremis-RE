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
#include "original_intro_player.h"
#include "player_death.h"
#include "original_ending_player.h"
#include "original_elevator_player.h"
#include "victory_session.h"
#include "shuttle_session.h"
#include "level22_communication.h"
#include "original_shuttle_player.h"

#include <exception>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string_view>

int main(int argc, char* argv[])
{
#ifdef EXTREMIS_DEBUG_MENU
    const bool communicationDebug = argc == 3 && std::string_view(argv[2]) == "--communication-debug";
    const bool shuttleDebug = argc == 3 && std::string_view(argv[2]) == "--shuttle-debug";
    const bool vitalsDebug = argc == 3 && std::string_view(argv[2]) == "--vitals-debug";
    const bool destructionDebug = argc == 3 && std::string_view(argv[2]) == "--self-destruction-debug";
#else
    const bool communicationDebug = false;
    const bool shuttleDebug = false;
    const bool vitalsDebug = false;
    const bool destructionDebug = false;
#endif
    if (argc > 3 || (argc == 3 && !vitalsDebug && !destructionDebug && !shuttleDebug && !communicationDebug)) {
        std::cerr << "Usage: " << argv[0] << " [path/to/GAME.LAB"
#ifdef EXTREMIS_DEBUG_MENU
                  << " [--vitals-debug|--self-destruction-debug|--shuttle-debug|--communication-debug]"
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
            data = game_lab::loadLevel(argv[1],(destructionDebug || shuttleDebug || communicationDebug)?21:0);
            if(destructionDebug) data->level.playerStart=game_lab::PlayerStart{0,280,300,360};
            if(communicationDebug) {
                data->level.playerStart=communication::returnPosition;
                data->level.playerStart->angle=540; // Face connection44; DOS return still uses 180.
            }
            if(shuttleDebug) {
                data->level.playerStart=shuttle::refusal;
                data->level.playerStart->angle=180; // Debug spawn faces the actual door.
            }
            const auto& level = data->level;
            items.load(level);
#ifdef EXTREMIS_DEBUG_MENU
            if(destructionDebug) {
                items.debugCompleteSecurities();
                SDL_Log("SELF-DESTRUCTION DEBUG: level22, real security flags completed, 48000 units. F12 shortens remaining budget to 1600; I/map/Debug/focus pause it.");
            }
#endif
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
    auto credits=communicationDebug?StartupCreditsResult::SkipAll:showStartupCredits(window,renderer,
        argc>=2 ? std::filesystem::path(argv[1]).parent_path() / "DATA"
                : std::filesystem::path{});
    if (credits==StartupCreditsResult::Error) {
        SDL_Log("Startup credits failed: %s",SDL_GetError());
        exitCode=1;
    }
    if (credits==StartupCreditsResult::Continue && data) {
        credits=showOriginalPresentation(window,renderer,std::filesystem::path(argv[1]).parent_path()/"DATA");
        if(credits==StartupCreditsResult::Error) exitCode=1;
    }
    if (credits==StartupCreditsResult::Continue && data) {
        credits=showOriginalIntro(window,renderer,std::filesystem::path(argv[1]).parent_path()/"DATA");
        if(credits==StartupCreditsResult::Error) exitCode=1;
    }
    if (credits==StartupCreditsResult::Continue || credits==StartupCreditsResult::SkipAll) {
        FirstPersonRenderer firstPerson(renderer);
        game_lab::Level emptyLevel;
        DoorControl doorControl(collisionWorld, levelDoors, data ? data->level : emptyLevel);
        PlayerInput playerInput(window, doorControl);
        GameAudio gameAudio;
        if (data) gameAudio.load(std::filesystem::path(argv[1]).parent_path() / "DATA");
        std::optional<player::DeathPresentation> deathPresentation;
        if(data && originalHud) {
            try { deathPresentation.emplace(player::loadDeathResources(std::filesystem::path(argv[1]).parent_path()/"DATA")); }
            catch(const std::exception& error) { SDL_Log("Original death presentation unavailable: %s",error.what()); }
        }
#ifdef EXTREMIS_DEBUG_MENU
        DebugMenu debugMenu;
        if (data) {
            try { debugMenu.load(argv[1]); }
            catch (const std::exception& error) { SDL_Log("Debug catalog: %s",error.what()); }
        }
#endif
        VictorySession victory;
        std::optional<ItemSession::Travel> shuttleTravel;
        bool running = true;
        bool showPolygons = false;
        bool show3D = camera.has_value();
        Uint64 previousTicks = SDL_GetTicks();
        while (running) {
            bool restartRequested=false;
            auto* activeEnemies=enemySystem?&*enemySystem:nullptr;
            bool debugPaused=false;
#ifdef EXTREMIS_DEBUG_MENU
            if (!debugMenu.enemiesEnabled()) activeEnemies=nullptr;
            debugPaused=debugMenu.open();
#endif
            const bool wasItemModal=items.modal();
            mapSession.beginFrame(data?data->level.zoneIndex:0);
            SDL_Event event;
            bool deathKey=false;
            while (!shuttleTravel && SDL_PollEvent(&event)) {
                if(deathPresentation && deathPresentation->active()) {
                    if(event.type==SDL_EVENT_QUIT || event.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED) running=false;
                    if(event.type==SDL_EVENT_KEY_DOWN && event.key.windowID==SDL_GetWindowID(window) && !event.key.repeat) {
                        deathKey=true;
                        const int direction=event.key.scancode==SDL_SCANCODE_UP?-1:event.key.scancode==SDL_SCANCODE_DOWN?1:0;
                        const bool confirm=event.key.scancode==SDL_SCANCODE_RETURN;
                        if(deathPresentation->stage()==player::DeathPresentation::Stage::Menu && (direction || confirm)) {
                            gameAudio.confirmItemSelection();
                            const auto request=deathPresentation->menuInput(direction,confirm);
                            if(request==player::DeathPresentation::Request::Restart) restartRequested=true;
                            if(request==player::DeathPresentation::Request::Exit) running=false;
                            if(request==player::DeathPresentation::Request::Unsupported)
                                SDL_Log("Native OPTIONS/password continuation is not implemented; RUN / Make new party starts a fresh game.");
                        }
                    }
                    // Existing explicit F10 diagnostic reset remains available;
                    // ordinary UI/use/movement must not consume death-screen keys.
                    if(event.type==SDL_EVENT_KEY_DOWN && event.key.scancode==SDL_SCANCODE_F10 && !victory.processed())
                        survival.handleEvent(event,SDL_GetWindowID(window));
                    playerInput.handleEvent(event,false);
                    continue;
                }
#ifdef EXTREMIS_DEBUG_MENU
                if(shuttleDebug && data && data->level.zoneIndex==21 &&
                   event.type==SDL_EVENT_KEY_DOWN && event.key.windowID==SDL_GetWindowID(window) &&
                   event.key.scancode==SDL_SCANCODE_L &&
                   (event.key.mod & SDL_KMOD_CTRL) && (event.key.mod & SDL_KMOD_SHIFT) && !event.key.repeat) {
                    std::uint16_t card=0x5005;
                    if(survival.inventory().takeSupply(card)==player::Transfer::Capacity)
                        survival.inventory().replaceCard(card,0);
                    items.debugCompleteSecurities();
                    SDL_Log("SHUTTLE DEBUG: card 0x2005 acquired and 16 securities disabled; approach the door again.");
                    continue;
                }
                if(destructionDebug && data && data->level.zoneIndex==21 &&
                   event.type==SDL_EVENT_KEY_DOWN && event.key.windowID==SDL_GetWindowID(window) &&
                   event.key.scancode==SDL_SCANCODE_F12 && !event.key.repeat) {
                    items.debugShortenDestruction();
                    SDL_Log("SELF-DESTRUCTION DEBUG: remaining %d units",items.selfDestruction().remaining());
                    continue;
                }
                if (data && event.type!=SDL_EVENT_QUIT && event.type!=SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                    !((event.type==SDL_EVENT_KEY_DOWN || event.type==SDL_EVENT_KEY_UP) && event.key.windowID!=SDL_GetWindowID(window)) &&
                    debugMenu.event(event,survival.inventory(),data->level.zoneIndex)) {
                    items.clearInput(); playerInput.handleEvent(event,false); playerInput.invalidateContext();
                    activeEnemies=debugMenu.enemiesEnabled() && enemySystem?&*enemySystem:nullptr;
                    continue;
                }
#endif
#ifdef EXTREMIS_DEBUG_MENU
                if(event.type==SDL_EVENT_KEY_DOWN && !event.key.repeat &&
                   event.key.windowID==SDL_GetWindowID(window) && event.key.scancode==SDL_SCANCODE_F9 &&
                   data && data->level.zoneIndex==29 && activeEnemies && !debugMenu.open()) {
                    for(std::size_t i=0;i<activeEnemies->records().size();++i)
                        while(activeEnemies->records()[i].health!=0xffff && activeEnemies->records()[i].state!=5)
                            activeEnemies->hit(i,4,0);
                    activeEnemies->takeSounds();
                    SDL_Log("Debug: lethal MONS9 hit; waiting for normal death completion");
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
            if(deathPresentation && deathPresentation->active() && !restartRequested) {
                if(survival.alive() && !victory.processed() && !items.selfDestruction().expired()) {
                    deathPresentation->reset(); gameAudio.stopEffects();
                    survival.update(0,false,gameAudio);
                    playerInput.invalidateContext(); doorControl.resetLevel();
                    SDL_SetWindowTitle(window,survival.windowTitle(texturePreview.windowTitle()).c_str());
                } else {
                    const auto* keys=SDL_GetKeyboardState(nullptr);
                    bool held=false; for(int i=0;i<SDL_SCANCODE_COUNT;++i) held|=keys[i];
                    if(SDL_GetKeyboardFocus()==window)
                        deathPresentation->advance(elapsedSeconds,deathKey,held);
                    if(deathPresentation->takeSoundStart()) gameAudio.beginDeath(
                        deathPresentation->cause()==player::DeathCause::Oxygen ? GameAudio::DeathSound::Oxygen :
                        deathPresentation->cause()==player::DeathCause::SelfDestruction ? GameAudio::DeathSound::SelfDestruction : GameAudio::DeathSound::Health);
                    if(deathPresentation->takeSoundStop()) gameAudio.stopEffects();
                    if(!SDL_SetRenderDrawColor(renderer,0,0,0,255) || !SDL_RenderClear(renderer) ||
                       !firstPerson.drawFrame(deathPresentation->pixels()) || !SDL_RenderPresent(renderer)) {
                        SDL_Log("Death presentation failed: %s",SDL_GetError()); exitCode=1; break;
                    }
                    // DOS body scroll has no explicit wait. The port uses 32 ms
                    // per step for visibility; original timing remains unverified.
                    const auto stage=deathPresentation->stage();
                    if(stage==player::DeathPresentation::Stage::Body) SDL_Delay(32);
                    else if(stage!=player::DeathPresentation::Stage::Menu) SDL_Delay(16);
                    continue; // No world, HUD, modal, weapon or door simulation.
                }
            }
#ifdef EXTREMIS_DEBUG_MENU
            debugPaused=debugMenu.open();
            activeEnemies=debugMenu.enemiesEnabled() && enemySystem?&*enemySystem:nullptr;
#endif
            const bool inputActive = camera && show3D && !debugPaused && SDL_GetKeyboardFocus() == window;
            // Resolve context changes before polling continuous WASD/Shift state.
            items.update(camera ? &*camera : nullptr,!shuttleTravel && inputActive && !mapSession.blocksSimulation(),
                         doorControl.movementWorld(),survival,gameAudio,&doorControl,elapsedSeconds);
                const bool arrivingByShuttle=shuttleTravel.has_value();
                auto travel=items.takeTravel();
                if(shuttleTravel) { travel=shuttleTravel;shuttleTravel.reset(); }
                if(restartRequested) travel=ItemSession::Travel{0,{0,300,320,180}};
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

                    const auto presentElevator=[&]() {
                        if(!travel->elevator) return true;
                        const auto& e=*travel->elevator;
                        const auto result=showOriginalElevator(window,renderer,
                            std::filesystem::path(argv[1]).parent_path()/"DATA",
                            int(e.destinationFloor)-int(e.sourceFloor),gameAudio);
                        playerInput.invalidateContext();items.clearInput();
                        if(result==StartupCreditsResult::Quit) running=false;
                        return result==StartupCreditsResult::Continue;
                    };
                    // An elevator travelling within the same level must not reload the world.
                    if (!restartRequested && data && travel->zone == data->level.zoneIndex) {
                        auto arrivalLevel = data->level;
                        arrivalLevel.playerStart = travel->arrival;

                        const auto nextCamera=initialCamera(arrivalLevel);
                        if(!presentElevator()) {
                            doorControl.resetLevel();previousTicks=SDL_GetTicks();continue;
                        }
                        camera = nextCamera;
                        playerInput.invalidateContext();

                        SDL_Log("Elevator: same-level transfer to sector %u",
                                travel->arrival.sector);

                        doorControl.resetLevel();
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
                    if(!presentElevator()) {
                        doorControl.resetLevel();previousTicks=SDL_GetTicks();continue;
                    }
                    if(restartRequested) {
                        survival=SurvivalSession(vitalsDebug);
                        items=ItemSession{};
                        items.setAvailable(interactionRenderer.has_value());
                        gameAudio.stopEffects(); deathPresentation->reset(); victory.reset();
#ifdef EXTREMIS_DEBUG_MENU
                        debugMenu=DebugMenu{}; debugMenu.load(argv[1]);
#endif
                    }
                    items.load(next.level); // New game resets plans; elevator travel keeps them.
                    if(arrivingByShuttle) {
                        SDL_ResetKeyboard();
                        SDL_FlushEvents(SDL_EVENT_KEY_DOWN,SDL_EVENT_KEY_UP);
                        SDL_FlushEvents(SDL_EVENT_MOUSE_MOTION,SDL_EVENT_MOUSE_WHEEL);
                        items.clearInput();
                    }
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
                doorControl.resetLevel();
                previousTicks=SDL_GetTicks();
                continue; // No simulation with elapsed time or input from the old world.
            }
            const bool itemTransition=wasItemModal!=items.modal();
            if (itemTransition) playerInput.invalidateContext();
            const bool modalBlocked = debugPaused || items.modal() || itemTransition || mapSession.blocksSimulation();
            const bool simulationActive = inputActive && !modalBlocked;
            mapSession.update(debugPaused?0:elapsedSeconds,SDL_GetKeyboardFocus()==window);
            texturePreview.update(modalBlocked?0:elapsedSeconds);
            if (spriteScene && !modalBlocked) spriteAnimation.update(*spriteScene, elapsedSeconds);
            if (spriteScene && simulationActive) decorativeAnimation.update(*spriteScene, elapsedSeconds);
            const Camera beforeMovement=camera.value_or(Camera{});
            playerInput.setAudioEnabled(gameAudio.enabled());
            playerInput.setObjectConstraint([&](const Camera& before, Camera& candidate) {
                if (!data) return;
                const auto location = collision::locate(data->level,{before.x,before.y},before.sector);
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

            const bool doorReady=doorControl.beginFrame(simulationActive && survival.alive() && playerInput.fireReady());
            const auto gameplayTicks=doorReady?doorControl.gameplayTicks():std::uint16_t(0);
            if (activeEnemies && camera && data)
                activeEnemies->update(data->level,*camera,doorControl.movementWorld(),survival.vitals(),
                    elapsedSeconds,simulationActive && survival.alive(),
                    [&](double seconds) {
                        playerInput.move(*camera,seconds,activeEnemies->reacting(),
                            [&](const Camera& before, Camera& after) {
                                activeEnemies->constrainPlayer(data->level,before,after,doorControl.staticWorld());
                            });
                    },
                    [&]() {
                        const auto before=*camera;
                        playerInput.recoil(*camera);
                        activeEnemies->constrainPlayer(data->level,before,*camera,doorControl.staticWorld());
                    },levelDoors,spriteScene ? &*spriteScene : nullptr);
            if(activeEnemies && activeEnemies->terminalDefeated()) {
                const auto result= victory.update(true,[&]() {
                    gameAudio.stopEffects(); activeEnemies->takeSounds();
                    items.clearInput(); playerInput.invalidateContext();
                    doorControl.beginFrame(false);
                    mapSession=original_map::Session{};
                    if(!deathPresentation) {
                        SDL_Log("Victory requires the original menu resources");
                        return StartupCreditsResult::Error;
                    }
                    original_ending::EntryImage entry;
                    entry.rgba=firstPerson.frame();
                    return showOriginalEnding(window,renderer,
                        std::filesystem::path(argv[1]).parent_path()/"DATA",std::move(entry));
                },[&]() { deathPresentation->beginMenu(); });
                previousTicks=SDL_GetTicks();
                SDL_ResetKeyboard(); items.clearInput(); playerInput.invalidateContext();
                if(result!=StartupCreditsResult::Continue) {
                    if(result==StartupCreditsResult::Error) exitCode=1;
                    break;
                }
                continue; // No remaining simulation, audio dispatch or gameplay rendering.
            }
            // One connection correction for the aggregate player update, never
            // inside the enemy/posture substeps. Then the common DOS door delta.
            if (simulationActive && survival.alive() && playerInput.fireReady() && camera) {
                if (doorControl.constrainPlayer(beforeMovement,*camera))
                    survival.postMessage(messages::Id::PortalLocked);
                if (doorReady) doorControl.updateDoors();
                else doorControl.rebuildWorld();
            }
            const double effectiveMovement=camera ? std::hypot(camera->x-beforeMovement.x,camera->y-beforeMovement.y) : 0;
            if (activeEnemies) for (const auto sound:activeEnemies->takeSounds()) gameAudio.enemyEffect(sound);
            if (survival.update(elapsedSeconds,simulationActive,gameAudio,playerInput.running() && effectiveMovement>0,
                                deathPresentation.has_value()))
                SDL_SetWindowTitle(window,survival.windowTitle(texturePreview.windowTitle()).c_str());
            const bool destructionRunning=doorReady && simulationActive && survival.alive() &&
                !items.busy() && items.selfDestruction().active();
            items.updateSelfDestruction(gameplayTicks,destructionRunning);
            gameAudio.destructionAlarm(destructionRunning && !items.selfDestruction().expired());
            if(destructionRunning) survival.postDestruction(items.selfDestruction().remaining());
            if(simulationActive && survival.alive() && !items.busy() && data && camera &&
               communication::trigger(data->level,camera->sector.value_or(~0u))) {
                gameAudio.stopEffects();items.clearInput();playerInput.invalidateContext();
                doorControl.beginFrame(false);doorControl.rebuildWorld();
                const auto result=communication::show(window,std::filesystem::path(argv[1]).parent_path()/"DATA",
                    [&](const std::vector<std::uint32_t>& pixels) {
                        return SDL_SetRenderDrawColor(renderer,0,0,0,255) && SDL_RenderClear(renderer) &&
                               firstPerson.drawFrame(pixels) && SDL_RenderPresent(renderer);
                    });
                auto returned=data->level;returned.playerStart=communication::returnPosition;
                const auto position=initialCamera(returned);
                camera->x=position.x;camera->y=position.y;camera->angle=position.angle;
                camera->sector=position.sector;
                survival.clearMessages();
                if(enemySystem) enemySystem->clearPlayerImpact();
                SDL_ResetKeyboard();items.clearInput();playerInput.invalidateContext();
                doorControl.resetLevel();previousTicks=SDL_GetTicks();
                if(result!=StartupCreditsResult::Continue) {
                    if(result==StartupCreditsResult::Error) exitCode=1;
                    break;
                }
                continue; // Discard modal elapsed time; no campaign simulation inside show().
            }
            if(simulationActive && survival.alive() && !items.busy() && data && camera &&
               shuttle::trigger(data->level,camera->sector.value_or(~0u))) {
                gameAudio.stopEffects();items.clearInput();playerInput.invalidateContext();
                doorControl.beginFrame(false);doorControl.rebuildWorld();
                shuttle::Session boarding;
                auto result=shuttle::prompt(window,renderer,std::filesystem::path(argv[1]).parent_path()/"DATA",
                                            boarding,survival.inventory(),items);
                if(result==StartupCreditsResult::Continue && boarding.step()==shuttle::Step::Flight) {
                    result=showOriginalShuttle(window,renderer,std::filesystem::path(argv[1]).parent_path()/"DATA");
                    if(result==StartupCreditsResult::Continue) {
                        shuttleTravel=boarding.complete(result,items);
                        survival.clearMessages();
                        SDL_Log("Shuttle complete: destination level 23, sector 1, local (320,300), angle 0, floor %u",shuttle::arrivalFloor);
                    }
                }
                if(!shuttleTravel) {
                    auto returned=data->level;returned.playerStart=shuttle::refusal;
                    camera=initialCamera(returned);
                }
                if(enemySystem) enemySystem->clearPlayerImpact();
                SDL_ResetKeyboard();items.clearInput();playerInput.invalidateContext();
                doorControl.resetLevel();previousTicks=SDL_GetTicks();
                if(result!=StartupCreditsResult::Continue) {
                    if(result==StartupCreditsResult::Error) exitCode=1;
                    break;
                }
                continue;
            }
            const bool shipExpired=items.selfDestruction().expired();
            const auto deathCause=!survival.alive()?survival.vitals().deathCause():
                shipExpired?player::DeathCause::SelfDestruction:player::DeathCause::None;
            if(shipExpired && !deathPresentation) {
                SDL_Log("Ship expiry requires original death resources");exitCode=1;break;
            }
            if(deathCause!=player::DeathCause::None && deathPresentation && !deathPresentation->active() && firstPerson.frame().size()==64000) {
                gameAudio.stopEffects();
                if(deathCause==player::DeathCause::SelfDestruction) {
                    try { deathPresentation->loadExplosion(std::filesystem::path(argv[1]).parent_path()/"DATA"); }
                    catch(const std::exception& error) {
                        SDL_Log("Ship explosion unavailable: %s",error.what());exitCode=1;break;
                    }
                }
                deathPresentation->begin(deathCause,firstPerson.frame());
                items.clearInput(); playerInput.invalidateContext();
                doorControl.beginFrame(false);
                if(!SDL_SetRenderDrawColor(renderer,0,0,0,255) || !SDL_RenderClear(renderer) ||
                   !firstPerson.drawFrame(deathPresentation->pixels()) || !SDL_RenderPresent(renderer)) {
                    SDL_Log("Death presentation failed: %s",SDL_GetError()); exitCode=1; break;
                }
                continue;
            }
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
            gameAudio.update(data ? data->level : emptyLevel, levelDoors, playerInput.footsteps(),doorReady);
            doorControl.resetCounter(); // Separate from the earlier read; rendering belongs to the next interval.
            const auto shuttleDoorFade=[&]() {
                const auto amount=data && camera && show3D && !modalBlocked ?
                    (shuttle::doorFade(data->level,camera->sector.value_or(~0u)) +
                     communication::doorFade(data->level,camera->sector.value_or(~0u))):0;
                if(!amount) return true;
                auto faded=firstPerson.frame();
                for(auto& c:faded) c=(((c>>24)*(100-amount)/100)<<24)|
                    ((((c>>16)&255)*(100-amount)/100)<<16)|((((c>>8)&255)*(100-amount)/100)<<8)|255;
                return firstPerson.drawFrame(faded);
            };
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
                !shuttleDoorFade() ||
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
