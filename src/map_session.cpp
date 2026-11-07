#include "map_session.h"
#include "survival_session.h"
#include <SDL3/SDL.h>
#include <cmath>
#include <utility>

namespace original_map {
void Session::beginFrame(unsigned zone)
{
    transitioned_=false;
    if (zone_!=zone) { state_.reset(); ticks_=0; keys_.fill(false); transitioned_=true; }
    zone_=zone;
}
bool Session::handleEvent(const SDL_Event& e, std::uint32_t windowId, const Camera* camera,
                         bool gameplay, bool transferred, bool blocked, bool available,
                         SurvivalSession& player)
{
    if (e.type==SDL_EVENT_WINDOW_FOCUS_LOST && e.window.windowID==windowId) keys_.fill(false);
    if (e.type==SDL_EVENT_WINDOW_FOCUS_GAINED && e.window.windowID==windowId && modal())
        transitioned_=true; // Discard the unfocused interval of the marker clock.
    if (e.type==SDL_EVENT_MOUSE_BUTTON_DOWN || e.type==SDL_EVENT_MOUSE_BUTTON_UP)
        return e.button.windowID==windowId && blocksSimulation();
    if (e.type==SDL_EVENT_MOUSE_MOTION) return e.motion.windowID==windowId && blocksSimulation();
    if (e.type==SDL_EVENT_MOUSE_WHEEL) return e.wheel.windowID==windowId && blocksSimulation();
    if ((e.type!=SDL_EVENT_KEY_DOWN && e.type!=SDL_EVENT_KEY_UP) || e.key.windowID!=windowId) return false;
    const auto key=e.key.scancode;
    if (key<=SDL_SCANCODE_UNKNOWN || key>=SDL_SCANCODE_COUNT) return blocksSimulation();
    const bool opening=key==SDL_SCANCODE_M || key==SDL_SCANCODE_SEMICOLON;
    if (e.type==SDL_EVENT_KEY_UP) { keys_[key]=false; return blocksSimulation() || opening; }
    const bool held=std::exchange(keys_[key],true);
    if (modal()) {
        if ((opening || key==SDL_SCANCODE_SPACE) && !held && !e.key.repeat) {
            state_.reset(); transitioned_=true;
        }
        return true;
    }
    if (transitioned_) return true;
    if (!opening) return false;
    if (held || e.key.repeat || !gameplay || !camera || !player.alive()) return true;
    if (blocked) player.postMessage(messages::Id::MapBlocked);
    else if (!transferred) player.postMessage(messages::Id::MapUnavailable);
    else if (!available || zone_>=levelCount) SDL_Log("Original map resources unavailable; map remains closed.");
    else if (placement(1,1,*camera)) {
        player.clearMessages();
        state_=State{zone_,*camera,false}; ticks_=0; transitioned_=true;
    }
    return true;
}
void Session::update(double seconds, bool focused)
{
    if (!state_ || transitioned_ || !focused || !std::isfinite(seconds) || seconds<=0) return;
    // Original strict >20 at 160 nominal ticks/s: 21 ticks per toggle.
    // Retain remainder for render-rate independence instead of DOS polling overshoot.
    ticks_=std::fmod(ticks_+std::fmod(seconds,42.0/160)*160,42.0);
    state_->markerVisible=ticks_+1e-9>=21;
}
}
