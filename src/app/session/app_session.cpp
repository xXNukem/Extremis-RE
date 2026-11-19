#include "extremis/diagnostics/shot_trace.h"
#include "app_session.h"

#include <SDL3/SDL.h>

#include "extremis/render/first_person/presentation/first_person_view.h"
#include "platform/sdl/input/player_input.h"
#include "app/diagnostics/debug_menu.h"
#ifdef EXTREMIS_DEBUG_MENU
#include "../diagnostics/gameplay_fps.h"
#include "platform/sdl/video/frame_profiler.h" // VORTEX_DEBUG_PROFILER_V1
#endif
#include "extremis/world/objet_collision.h"
#include "extremis/audio/game_audio.h"
#include "extremis/cinematics/playback_result.h"
#include "app/startup/port_menu.h"
#include "extremis/cinematics/player_death.h"
#include "app/session/victory_session.h"
#include "app/session/shuttle_session.h"
#include "extremis/cinematics/level22_communication.h"

#include <exception>
#include <cmath>
#include <optional>

namespace app {
void AppSession::run(SDL_Window* window, SDL_Renderer* renderer, const LaunchOptions& options,
    const std::filesystem::path& dataDirectory, PortMenuVideoState& video, int& exitCode)
{
#ifdef EXTREMIS_DEBUG_MENU
    platform::sdl::video_profiling::FrameProfiler profiler;
    FirstPersonRenderer firstPerson(renderer,&video.render,&profiler);
#else
    FirstPersonRenderer firstPerson(renderer,&video.render);
#endif
    if (data) firstPerson.setRaycastLevel(data->level);
    game_lab::Level emptyLevel;
    DoorControl doorControl(collisionWorld, levelDoors, data ? data->level : emptyLevel);
    PlayerInput playerInput(window, doorControl);
    GameAudio gameAudio;
    if (data) gameAudio.load(std::filesystem::path(options.labPath).parent_path() / "DATA");
    std::optional<player::DeathPresentation> deathPresentation;
    if(data && originalHud) {
        try { deathPresentation.emplace(player::loadDeathResources(std::filesystem::path(options.labPath).parent_path()/"DATA")); }
        catch(const std::exception& error) { SDL_Log("Original death presentation unavailable: %s",error.what()); }
    }
#ifdef EXTREMIS_DEBUG_MENU
    DebugMenu debugMenu;
    if (data) {
        try { debugMenu.load(options.labPath); }
        catch (const std::exception& error) { SDL_Log("Debug catalog: %s",error.what()); }
    }
#endif
    VictorySession victory;
    std::optional<ItemSession::Travel> shuttleTravel;
    bool running = true;
    bool showPolygons = false;
    bool show3D = camera.has_value();
    Uint64 previousTicks = SDL_GetTicks();
#ifdef EXTREMIS_DEBUG_MENU
    // VORTEX_DEBUG_FPS_V1: FPS of actually presented gameplay frames.
    diagnostics::GameplayFps fps;
#endif
    if (extremis::shot_trace::mode) {
        int vsync=0;
        const bool known=SDL_GetRenderVSync(renderer,&vsync);
        const auto* display=SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(window));
        extremis::shot_trace::event("renderer", "name=%s vsync=%d known=%d refresh_hz=%.3f",
                                    SDL_GetRendererName(renderer),vsync,known,display?display->refresh_rate:0.0);
    }
    while (running) {
        if (extremis::shot_trace::mode) ++extremis::shot_trace::frame;
#ifdef EXTREMIS_DEBUG_MENU
        profiler.beginFrame();
#endif
        bool restartRequested=false;
        auto* activeEnemies=enemySystem?&*enemySystem:nullptr;
        bool debugPaused=false;
#ifdef EXTREMIS_DEBUG_MENU
        if (!debugMenu.enemiesEnabled()) activeEnemies=nullptr;
        debugPaused=debugMenu.open();
#endif
        const bool wasItemModal=items.modal();
        mapSession.beginFrame(data?data->level.zoneIndex:0);
        EventBatch events{true, false, false, showPolygons, activeEnemies};
        if (!shuttleTravel) {
            events=pollEvents(window,renderer,options,dataDirectory,video,exitCode,previousTicks,
                playerInput,gameAudio,deathPresentation ? &*deathPresentation : nullptr,
                victory,show3D,events
#ifdef EXTREMIS_DEBUG_MENU
                , debugMenu,profiler,fps
#endif
            );
        }
        running=events.running;
        restartRequested=events.restartRequested;
        showPolygons=events.showPolygons;
        activeEnemies=events.activeEnemies;

        if (!running) {
            break;
        }

        const Uint64 ticks = SDL_GetTicks();
        const double elapsedSeconds = double(ticks - previousTicks) / 1000.0;
        previousTicks = ticks;
        if (deathPresentation && deathPresentation->active() && !restartRequested) {
            const auto death=advanceDeathPresentation(window,renderer,firstPerson,*deathPresentation,
                gameAudio,playerInput,doorControl,victory,elapsedSeconds,events.deathKey);
            if (death==DeathFrame::Error) { exitCode=1; break; }
            if (death==DeathFrame::Presented) continue;
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
            transferLevel(window, renderer, options, *travel, restartRequested, arrivingByShuttle,
                running, firstPerson, doorControl, playerInput, gameAudio,
                deathPresentation ? &*deathPresentation : nullptr, victory
#ifdef EXTREMIS_DEBUG_MENU
                , debugMenu
#endif
            );
            previousTicks=SDL_GetTicks();
            continue; // Never simulate with elapsed time or input from the old world.
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
            const auto result=playVictory(window,renderer,options.labPath,firstPerson,
                gameAudio,playerInput,doorControl,*activeEnemies,
                deathPresentation ? &*deathPresentation : nullptr,victory);
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
            const auto result=playCommunication(window,renderer,options.labPath,
                firstPerson,gameAudio,playerInput,doorControl,previousTicks);
            if(result!=StartupCreditsResult::Continue) {
                if(result==StartupCreditsResult::Error) exitCode=1;
                break;
            }
            continue; // Discard modal elapsed time; no campaign simulation inside show().
        }
        if(simulationActive && survival.alive() && !items.busy() && data && camera &&
           shuttle::trigger(data->level,camera->sector.value_or(~0u))) {
            const auto result=playShuttle(window,renderer,options.labPath,
                gameAudio,playerInput,doorControl,shuttleTravel);
            doorControl.resetLevel();previousTicks=SDL_GetTicks();
            if(result!=StartupCreditsResult::Continue) {
                if(result==StartupCreditsResult::Error) exitCode=1;
                break;
            }
            continue;
        }
        const auto death=beginDeathPresentation(renderer,options.labPath,firstPerson,
            deathPresentation ? &*deathPresentation : nullptr,gameAudio,playerInput,doorControl);
        if (death==DeathFrame::Error) { exitCode=1; break; }
        if (death==DeathFrame::Presented) continue;
        const auto trigger=playerInput.takeFireTrigger();
        const bool weaponsActive=simulationActive && survival.alive() && playerInput.fireReady() &&
                                 originalHud && weaponGraphicsReady;
        extremis::shot_trace::event("update", "dt_ms=%.3f gameplay_ticks=%u active=%d modal=%d held=%d pressed=%d level=%u",
                     elapsedSeconds*1000, unsigned(gameplayTicks), weaponsActive, modalBlocked,
                     trigger.held,trigger.pressed,data?unsigned(data->level.zoneIndex)+1:0);
        const auto fired=weaponFire.update(camera?*camera:Camera{},doorControl.staticWorld(),
            survival.inventory(),trigger,weaponsActive,
            elapsedSeconds,activeEnemies,doorControl.projectileLeaves());
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
        if (!presentFrame(renderer,firstPerson,hudState,activeEnemies,
                          show3D,showPolygons,modalBlocked
#ifdef EXTREMIS_DEBUG_MENU
                          ,debugMenu,fps.value(),profiler
#endif
                          )) {
            SDL_Log("Rendering failed: %s", SDL_GetError());
            exitCode = 1;
            break;
        }

#ifdef EXTREMIS_DEBUG_MENU
        // Sample after SDL_RenderPresent, so its cost is reflected in FPS.
        if (data && show3D && camera && !debugMenu.open() && !mapSession.state())
            fps.recordPresent(SDL_GetTicksNS());
        else
            fps.resetInterval();
        if (data && show3D && camera && !debugMenu.open() && !mapSession.state())
            profiler.endFrame();
        else
            profiler.reset();
#endif
        // No fixed SDL_Delay: SDL/presentation controls any necessary pacing.
    }
}
}

namespace app {
bool AppSession::hasLevel() const
{
    return data.has_value();
}

std::string AppSession::windowTitle() const
{
    return survival.windowTitle(texturePreview.windowTitle());
}
} // namespace app

namespace app {

AppSession::AppSession(bool vitalsDebug) : survival(vitalsDebug)
{}

} // namespace app
