#include "app_session.h"
#include "destination_level.h"

#include "app/diagnostics/debug_menu.h"
#include "app/session/victory_session.h"
#include "extremis/cinematics/playback_result.h"
#include "extremis/audio/game_audio.h"
#include "extremis/cinematics/original_elevator_player.h"
#include "extremis/cinematics/player_death.h"
#include "extremis/render/first_person/camera/camera_start.h"
#include "extremis/render/first_person/presentation/first_person_view.h"
#include "platform/sdl/input/player_input.h"

#include <SDL3/SDL.h>
#include <exception>
#include <utility>

namespace app {
void AppSession::transferLevel(SDL_Window* window, SDL_Renderer* renderer,
    const LaunchOptions& options, const ItemSession::Travel& travel,
    bool restartRequested, bool arrivingByShuttle, bool& running,
    FirstPersonRenderer& firstPerson, DoorControl& doorControl, PlayerInput& playerInput,
    GameAudio& gameAudio, player::DeathPresentation* deathPresentation, VictorySession& victory
#ifdef EXTREMIS_DEBUG_MENU
    , DebugMenu& debugMenu
#endif
)
{
    const auto presentElevator=[&]() {
        if(!travel.elevator) return true;
        const auto& e=*travel.elevator;
        const auto result=showOriginalElevator(window,renderer,
            std::filesystem::path(options.labPath).parent_path()/"DATA",
            int(e.destinationFloor)-int(e.sourceFloor),gameAudio);
        playerInput.invalidateContext();items.clearInput();
        if(result==StartupCreditsResult::Quit) running=false;
        return result==StartupCreditsResult::Continue;
    };
    // An elevator travelling within the same level must not reload the world.
    if (!restartRequested && data && travel.zone == data->level.zoneIndex) {
        auto arrivalLevel = data->level;
        arrivalLevel.playerStart = travel.arrival;

        const auto nextCamera=initialCamera(arrivalLevel);
        if(!presentElevator()) {
            doorControl.resetLevel();
            return;
        }
        camera = nextCamera;
        playerInput.invalidateContext();

        SDL_Log("Elevator: same-level transfer to sector %u",
                travel.arrival.sector);

        doorControl.resetLevel();
        return;
    }

    try {
        auto destination=prepareDestination(options.labPath,travel.zone,travel.arrival);
        if(!presentElevator()) {
            doorControl.resetLevel();
            return;
        }
        if(restartRequested) {
            survival=SurvivalSession(options.vitalsDebug);
            items=ItemSession{};
            items.setAvailable(interactionRenderer.has_value());
            gameAudio.stopEffects();
            if (deathPresentation) deathPresentation->reset();
            victory.reset();
#ifdef EXTREMIS_DEBUG_MENU
            debugMenu=DebugMenu{}; debugMenu.load(options.labPath);
#endif
        }
        items.load(destination.levelData.level); // New game resets plans; elevator travel keeps them.
        if(arrivingByShuttle) {
            SDL_ResetKeyboard();
            SDL_FlushEvents(SDL_EVENT_KEY_DOWN,SDL_EVENT_KEY_UP);
            SDL_FlushEvents(SDL_EVENT_MOUSE_MOTION,SDL_EVENT_MOUSE_WHEEL);
            items.clearInput();
        }
        // Assign into engaged storage: DoorControl borrows data->level.
        data=std::move(destination.levelData); camera=destination.camera;
        firstPerson.setRaycastLevel(data->level);
        collisionWorld=std::move(destination.world); levelDoors=std::move(destination.doors);
        doorControl.resetLevel();
        spriteScene=std::move(destination.sprites); spriteAnimation=sprites::Animation{};
        decorativeAnimation=sprites::DecorativeAnimation{};
        enemySystem=std::move(destination.enemies); originalHud=std::move(destination.hud);
        weaponFire=std::move(destination.weapon); weaponGraphicsReady=true;
        texturePreview=std::move(destination.textures); planeAppearance=std::move(destination.planes);
        mapSession=original_map::Session{};
        mapSession.beginFrame(data->level.zoneIndex);
        survival.clearMessages(); // Vitals, inventory and battery retain their values.
        playerInput.invalidateContext();
        SDL_SetWindowTitle(window,survival.windowTitle(texturePreview.windowTitle()).c_str());
        SDL_Log("Elevator: level %u, sector %u, arrival %.0f,%.0f; %zu sectors, %zu sprites, %zu enemies, %zu doors",
            travel.zone+1,travel.arrival.sector,camera->x,camera->y,
            data->level.sectors.size(),spriteScene->objects.size(),
            enemySystem->records().size(),levelDoors.size());
    } catch (const std::exception& error) {
        SDL_Log("Elevator destination load failed: %s",error.what());
    }
    doorControl.resetLevel();
}
}
