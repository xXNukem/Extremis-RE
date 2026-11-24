#include <cstdint>
#include <filesystem>
#include <optional>
#include <utility>
#include <vector>
#include "extremis/render/first_person/camera/camera_start.h"
#include "app_session.h"

#include "extremis/world/door_control.h"
#include "extremis/render/first_person/presentation/first_person_view.h"
#include "extremis/audio/game_audio.h"
#include "extremis/cinematics/level22_communication.h"
#include "extremis/cinematics/original_shuttle_player.h"
#include "extremis/cinematics/original_ending_player.h"
#include "platform/sdl/input/player_input.h"
#include "app/session/shuttle_session.h"

#include <SDL3/SDL.h>

namespace app {
StartupCreditsResult AppSession::playCommunication(SDL_Window* window, SDL_Renderer* renderer,
    const char* labPath, FirstPersonRenderer& firstPerson, GameAudio& gameAudio,
    PlayerInput& playerInput, DoorControl& doorControl, std::uint64_t& previousTicks)
{
    gameAudio.stopEffects();items.clearInput();playerInput.invalidateContext();
    doorControl.beginFrame(false);doorControl.rebuildWorld();
    const auto result=communication::show(window,std::filesystem::u8path(labPath).parent_path()/"DATA",
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
    return result;
}

StartupCreditsResult AppSession::playShuttle(SDL_Window* window, SDL_Renderer* renderer,
    const char* labPath, GameAudio& gameAudio, PlayerInput& playerInput,
    DoorControl& doorControl, std::optional<ItemSession::Travel>& shuttleTravel)
{
    gameAudio.stopEffects();items.clearInput();playerInput.invalidateContext();
    doorControl.beginFrame(false);doorControl.rebuildWorld();
    shuttle::Session boarding;
    auto result=shuttle::prompt(window,renderer,std::filesystem::u8path(labPath).parent_path()/"DATA",
                                boarding,survival.inventory(),items);
    if(result==StartupCreditsResult::Continue && boarding.step()==shuttle::Step::Flight) {
        result=showOriginalShuttle(window,renderer,std::filesystem::u8path(labPath).parent_path()/"DATA",
                                   false,gameAudio.outputGain());
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
    return result;
}

StartupCreditsResult AppSession::playVictory(SDL_Window* window, SDL_Renderer* renderer,
    const char* labPath, FirstPersonRenderer& firstPerson, GameAudio& gameAudio,
    PlayerInput& playerInput, DoorControl& doorControl, enemies::System& activeEnemies)
{
    gameAudio.stopEffects(); activeEnemies.takeSounds();
    items.clearInput(); playerInput.invalidateContext();
    doorControl.beginFrame(false);
    mapSession=original_map::Session{};
    original_ending::EntryImage entry;
    entry.rgba=firstPerson.frame();
    return showOriginalEnding(window,renderer,
        std::filesystem::u8path(labPath).parent_path()/"DATA",std::move(entry),gameAudio.outputGain());
}
}
