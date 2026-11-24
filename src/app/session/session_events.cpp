#include <cstddef>
#include <cstdint>
#include <filesystem>
#include "extremis/diagnostics/shot_trace.h"
#include "app_session.h"

#include "app/diagnostics/debug_menu.h"
#include "app/session/victory_session.h"
#include "app/startup/port_menu.h"
#include "extremis/audio/game_audio.h"
#include "extremis/cinematics/original_intro_player.h"
#include "extremis/cinematics/player_death.h"
#include "extremis/world/door_control.h"
#include "platform/sdl/input/player_input.h"
#include "platform/sdl/input/survival_input.h"
#ifdef EXTREMIS_DEBUG_MENU
#include "../diagnostics/gameplay_fps.h"
#include "platform/sdl/video/frame_profiler.h"
#endif

#include <SDL3/SDL.h>

namespace app {
AppSession::EventBatch AppSession::pollEvents(SDL_Window* window, SDL_Renderer* renderer,
    [[maybe_unused]] const LaunchOptions& options, const std::filesystem::path& dataDirectory,
    UserSettings& settings, const std::filesystem::path& settingsPath, int& exitCode, std::uint64_t& previousTicks,
    PlayerInput& playerInput, GameAudio& gameAudio, const DoorControl& doorControl,
    player::DeathPresentation* deathPresentation,
    const VictorySession& victory, bool show3D, EventBatch result
#ifdef EXTREMIS_DEBUG_MENU
    , DebugMenu& debugMenu, platform::sdl::video_profiling::FrameProfiler& profiler,
    diagnostics::GameplayFps& fps
#endif
)
{
    const auto windowId=SDL_GetWindowID(window);
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type==SDL_EVENT_RENDER_DEVICE_RESET &&
            event.render.windowID==windowId)
            applyPreferredVSync(renderer,settings);
        if (extremis::shot_trace::mode &&
            (event.type==SDL_EVENT_MOUSE_BUTTON_DOWN || event.type==SDL_EVENT_MOUSE_BUTTON_UP) &&
            event.button.button==SDL_BUTTON_LEFT && event.button.windowID==SDL_GetWindowID(window)) {
            const auto now=SDL_GetTicksNS();
            const auto age=event.button.timestamp && event.button.timestamp<=now ?
                static_cast<double>(now-event.button.timestamp)*1e-6 : -1.;
            extremis::shot_trace::event("input", "down=%d queue_ms=%.3f",
                event.type==SDL_EVENT_MOUSE_BUTTON_DOWN,age);
        }
        if(deathPresentation && deathPresentation->active()) {
            if(event.type==SDL_EVENT_QUIT || event.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED) result.running=false;
            if(event.type==SDL_EVENT_KEY_DOWN && event.key.windowID==SDL_GetWindowID(window) && !event.key.repeat) {
                result.deathKey=true;
                // Keep the opt-in diagnostic reset available during death.
                if(event.key.scancode==SDL_SCANCODE_F10 && !victory.processed())
                    survival.handleInput(platform::sdl::survivalInput(event,SDL_GetWindowID(window)));
            }
            playerInput.handleEvent(event,false);
            continue;
        }
        // During play, Escape opens the port menu, including when a terminal or
        // inventory screen is open. It no longer cancels those screens.
        const bool pressed=event.type==SDL_EVENT_KEY_DOWN && !event.key.repeat && event.key.windowID==windowId;
        const bool classicGameplay=settings.classicControls && show3D && survival.alive() &&
            !items.busy() && !mapSession.blocksSimulation() && SDL_GetKeyboardFocus()==window
#ifdef EXTREMIS_DEBUG_MENU
            && !debugMenu.open()
#endif
            ;
        if (pressed && (event.key.scancode==SDL_SCANCODE_ESCAPE ||
            (classicGameplay && event.key.scancode==SDL_SCANCODE_P))) {
            openMenu(window,renderer,dataDirectory,settings,settingsPath,exitCode,previousTicks,playerInput,gameAudio,true,result);
#ifdef EXTREMIS_DEBUG_MENU
            profiler.reset();
            fps.resetInterval();
#endif
            break; // The old game never consumes menu keys in this frame.
        }
#ifdef EXTREMIS_DEBUG_MENU
        if(options.shuttleDebug && data && data->level.zoneIndex==21 &&
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
        if(options.destructionDebug && data && data->level.zoneIndex==21 &&
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
            result.activeEnemies=debugMenu.enemiesEnabled() && enemySystem?&*enemySystem:nullptr;
            continue;
        }
#endif
#ifdef EXTREMIS_DEBUG_MENU
        if(event.type==SDL_EVENT_KEY_DOWN && !event.key.repeat &&
           event.key.windowID==SDL_GetWindowID(window) && event.key.scancode==SDL_SCANCODE_F9 &&
           data && data->level.zoneIndex==29 && result.activeEnemies && !debugMenu.open()) {
            for(std::size_t i=0;i<result.activeEnemies->records().size();++i)
                while(result.activeEnemies->records()[i].health!=0xffff && result.activeEnemies->records()[i].state!=5)
                    result.activeEnemies->hit(i,4,0);
            result.activeEnemies->takeSounds();
            SDL_Log("Debug: lethal MONS9 hit; waiting for normal death completion");
            continue;
        }
#endif
        const bool wasMap=mapSession.modal();
        const bool mapKey=event.type==SDL_EVENT_KEY_DOWN &&
            (event.key.scancode==SDL_SCANCODE_M || event.key.scancode==SDL_SCANCODE_SEMICOLON);
        const bool mapBlocked=mapKey && result.activeEnemies && camera && data &&
            result.activeEnemies->blocksInterface(data->level,*camera,levelDoors);
        const bool mapEvent=mapSession.handleEvent(event,SDL_GetWindowID(window),
            camera?&*camera:nullptr,show3D && !items.busy() && SDL_GetKeyboardFocus()==window,
            items.planTransferred(),mapBlocked,originalMap.has_value() && originalHud.has_value(),survival);
        if (wasMap!=mapSession.modal()) { items.clearInput(); playerInput.invalidateContext(); }
        // Releases must clear held UI keys even while the map owns input.
        const bool itemEvent=(!mapEvent || event.type==SDL_EVENT_KEY_UP) &&
            items.handleEvent(event,SDL_GetWindowID(window));
        const bool modalEvent=mapEvent || itemEvent;
        // Observe releases even if the UI owns the event, but never arm UI clicks.
        const bool gameplay=!modalEvent && !items.modal() && !mapSession.blocksSimulation() && show3D && survival.alive();
        if (playerInput.handleEvent(event,gameplay) && classicGameplay && camera &&
            items.requestContextualAction(*camera,doorControl.movementWorld()))
            playerInput.suppressClassicFire();
        if (modalEvent) continue;
        if ((event.type==SDL_EVENT_KEY_DOWN || event.type==SDL_EVENT_KEY_UP) &&
            event.key.windowID!=SDL_GetWindowID(window)) continue;
        survival.handleInput(platform::sdl::survivalInput(event,SDL_GetWindowID(window)));
        if (pressed && event.key.scancode==SDL_SCANCODE_F) {
            auto& enabled=settings.render.preferences.floorAndCeiling;
            enabled=!enabled;
            saveUserSettings(settingsPath,settings);
            SDL_Log("Floor/ceiling: %s",enabled ? "textured" : "flat colors");
        }
        if (pressed && classicGameplay && event.key.scancode==SDL_SCANCODE_S) gameAudio.toggleEffects();
#ifdef EXTREMIS_DEBUG_MENU
        if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat &&
            event.key.windowID==SDL_GetWindowID(window) &&
            (event.key.scancode == SDL_SCANCODE_PAGEDOWN || event.key.scancode == SDL_SCANCODE_PAGEUP)) {
            texturePreview.changeIndex(event.key.scancode == SDL_SCANCODE_PAGEUP ? 1 : -1);
            SDL_SetWindowTitle(window, survival.windowTitle(texturePreview.windowTitle()).c_str());
        }
        if (!settings.classicControls && event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_P &&
            !event.key.repeat) {
            result.showPolygons = !result.showPolygons;
            SDL_Log("Polygon contours: %s", result.showPolygons ? "on" : "off");
        }
#endif
        if (event.type == SDL_EVENT_QUIT ||
            event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            result.running = false;
        }
    }
    return result;
}

void AppSession::openMenu(SDL_Window* window, SDL_Renderer* renderer,
    const std::filesystem::path& dataDirectory, UserSettings& settings, const std::filesystem::path& settingsPath,
    int& exitCode, std::uint64_t& previousTicks, PlayerInput& playerInput,
    GameAudio& gameAudio, bool canResume, EventBatch& result)
{
    if (!canResume) {
        gameAudio.stopEffects();
        SDL_ResetKeyboard();
        SDL_FlushEvents(SDL_EVENT_KEY_DOWN,SDL_EVENT_KEY_UP);
    }
    gameAudio.pause(true);
    playerInput.releaseRelativeMouse();
    const auto menu=showPortMenu(window,renderer,dataDirectory,canResume,settings,settingsPath,
        [&](float gain) { gameAudio.setMasterGain(gain); });
    const auto choice=menu.choice;
    gameAudio.setMasterGain(settings.audioGain());
    gameAudio.pause(false);
    playerInput.setClassicControls(settings.classicControls);
    items.clearInput();mapSession.clearInput();playerInput.invalidateContext();
    SDL_ResetKeyboard();
    SDL_FlushEvents(SDL_EVENT_KEY_DOWN,SDL_EVENT_KEY_UP);
    previousTicks=SDL_GetTicks(); // Never simulate time spent in the menu.
    if (choice==PortMenuChoice::Exit || choice==PortMenuChoice::Error) {
        if (choice==PortMenuChoice::Error) exitCode=1;
        result.running=false;
    } else if (choice==PortMenuChoice::Continue && data) {
        result.checkpoint=menu.checkpoint;
        result.restartRequested=true;
    } else if (choice==PortMenuChoice::NewGame && data) {
        gameAudio.stopEffects();
        auto intro=showOriginalPresentation(window,renderer,dataDirectory);
        if (intro==StartupCreditsResult::Continue)
            intro=showOriginalIntro(window,renderer,dataDirectory,settings.audioGain());
        if (intro==StartupCreditsResult::Quit || intro==StartupCreditsResult::Error) {
            if (intro==StartupCreditsResult::Error) exitCode=1;
            result.running=false;
        } else result.restartRequested=true;
        previousTicks=SDL_GetTicks();
        SDL_ResetKeyboard();
        SDL_FlushEvents(SDL_EVENT_KEY_DOWN,SDL_EVENT_KEY_UP);
    }
}
}
