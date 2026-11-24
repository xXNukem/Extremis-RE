#include "app_session.h"

#include "app/session/victory_session.h"
#include "extremis/audio/game_audio.h"
#include "extremis/cinematics/player_death.h"
#include "extremis/render/first_person/presentation/first_person_view.h"
#include "platform/sdl/input/player_input.h"

#include <SDL3/SDL.h>
#include <exception>

namespace app {
AppSession::DeathFrame AppSession::advanceDeathPresentation(SDL_Window* window, SDL_Renderer* renderer,
    FirstPersonRenderer& firstPerson, player::DeathPresentation& deathPresentation,
    GameAudio& gameAudio, PlayerInput& playerInput, DoorControl& doorControl,
    const VictorySession& victory, double elapsedSeconds, bool deathKey)
{
    if(survival.alive() && !victory.processed() && !items.selfDestruction().expired()) {
        deathPresentation.reset(); gameAudio.stopEffects();
        survival.update(0,false,gameAudio);
        playerInput.invalidateContext(); doorControl.resetLevel();
        SDL_SetWindowTitle(window,survival.windowTitle(texturePreview.windowTitle()).c_str());
        return DeathFrame::Inactive;
    }

    const auto* keys=SDL_GetKeyboardState(nullptr);
    bool held=false; for(int i=0;i<SDL_SCANCODE_COUNT;++i) held|=keys[i];
    if(SDL_GetKeyboardFocus()==window)
        deathPresentation.advance(elapsedSeconds,deathKey,held);
    if(deathPresentation.takeSoundStart()) gameAudio.beginDeath(
        deathPresentation.cause()==player::DeathCause::Oxygen ? GameAudio::DeathSound::Oxygen :
        deathPresentation.cause()==player::DeathCause::SelfDestruction ? GameAudio::DeathSound::SelfDestruction : GameAudio::DeathSound::Health);
    if(deathPresentation.takeSoundStop()) gameAudio.stopEffects();
    if(!SDL_SetRenderDrawColor(renderer,0,0,0,255) || !SDL_RenderClear(renderer) ||
       !firstPerson.drawFrame(deathPresentation.pixels()) || !SDL_RenderPresent(renderer)) {
        SDL_Log("Death presentation failed: %s",SDL_GetError()); return DeathFrame::Error;
    }
    // DOS body scroll has no explicit wait. The port uses 32 ms
    // per step for visibility; original timing remains unverified.
    const auto stage=deathPresentation.stage();
    if(stage==player::DeathPresentation::Stage::Body) SDL_Delay(32);
    else SDL_Delay(16);
    return stage==player::DeathPresentation::Stage::Finished ? DeathFrame::Finished : DeathFrame::Presented;
}

AppSession::DeathFrame AppSession::beginDeathPresentation(SDL_Renderer* renderer,
    const char* labPath, FirstPersonRenderer& firstPerson, player::DeathPresentation* deathPresentation,
    GameAudio& gameAudio, PlayerInput& playerInput, DoorControl& doorControl)
{
    const bool shipExpired=items.selfDestruction().expired();
    const auto deathCause=!survival.alive()?survival.vitals().deathCause():
        shipExpired?player::DeathCause::SelfDestruction:player::DeathCause::None;
    if(deathCause!=player::DeathCause::None && !deathPresentation) return DeathFrame::Finished;
    if(deathCause!=player::DeathCause::None && deathPresentation && !deathPresentation->active() && firstPerson.frame().size()==64000) {
        gameAudio.stopEffects();
        if(deathCause==player::DeathCause::SelfDestruction) {
            try { deathPresentation->loadExplosion(std::filesystem::path(labPath).parent_path()/"DATA"); }
            catch(const std::exception& error) {
                SDL_Log("Ship explosion unavailable: %s",error.what());return DeathFrame::Error;
            }
        }
        deathPresentation->begin(deathCause,firstPerson.frame());
        items.clearInput(); playerInput.invalidateContext();
        doorControl.beginFrame(false);
        if(!SDL_SetRenderDrawColor(renderer,0,0,0,255) || !SDL_RenderClear(renderer) ||
           !firstPerson.drawFrame(deathPresentation->pixels()) || !SDL_RenderPresent(renderer)) {
            SDL_Log("Death presentation failed: %s",SDL_GetError()); return DeathFrame::Error;
        }
        return DeathFrame::Presented;
    }
    return DeathFrame::Inactive;
}
}
