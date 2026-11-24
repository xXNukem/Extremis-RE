#include <cstdint>
#include "app/session/item_session.h"

#include <SDL3/SDL_events.h>
#include <utility>

bool ItemSession::requestContextualAction(const Camera& camera, const collision::World& obstacles)
{
    if (busy() || !available_ || !world_ || !world_->target(camera,obstacles)) return false;
    take_ = true;
    return true;
}

bool ItemSession::handleEvent(const SDL_Event& event, std::uint32_t windowId)
{
    if (event.type==SDL_EVENT_WINDOW_FOCUS_LOST && event.window.windowID==windowId) {
        action_=Action::None; inventoryRequested_=take_=injector_=oxygen_=false;
        resumeAnimation_=true; keysDown_.fill(false);
        return false;
    }
    if (event.type==SDL_EVENT_MOUSE_BUTTON_DOWN || event.type==SDL_EVENT_MOUSE_BUTTON_UP)
        return event.button.windowID==windowId && (modal() || inventoryRequested_);
    if (event.type==SDL_EVENT_MOUSE_MOTION)
        return event.motion.windowID==windowId && (modal() || inventoryRequested_);
    if (event.type==SDL_EVENT_MOUSE_WHEEL)
        return event.wheel.windowID==windowId && (modal() || inventoryRequested_);
    if ((event.type!=SDL_EVENT_KEY_DOWN && event.type!=SDL_EVENT_KEY_UP) ||
        event.key.windowID!=windowId) return false;
    const auto key=event.key.scancode;
    if (key<=SDL_SCANCODE_UNKNOWN || key>=SDL_SCANCODE_COUNT) return modal();
    const bool gameplayAction=key==SDL_SCANCODE_E || key==SDL_SCANCODE_I ||
                              key==SDL_SCANCODE_Z || key==SDL_SCANCODE_X;
    if (event.type==SDL_EVENT_KEY_UP) {
        keysDown_[key]=false;
        return modal() || inventoryRequested_ || gameplayAction;
    }
    const bool held=std::exchange(keysDown_[key],true);
    if (held || event.key.repeat) return modal() || gameplayAction;
    if (!modal() && inventoryRequested_) {
        // I and Escape may arrive in one SDL pump before the opening update.
        if (key==SDL_SCANCODE_I || key==SDL_SCANCODE_ESCAPE)
            inventoryRequested_=take_=injector_=oxygen_=false;
        return true;
    }
    if (!modal() && take_ && key==SDL_SCANCODE_ESCAPE) { take_=false; return true; }
    if (modal()) {
        // Closing takes precedence over other keys queued in the same event pump.
        if (key==SDL_SCANCODE_ESCAPE || key==SDL_SCANCODE_BACKSPACE ||
            (key==SDL_SCANCODE_I && interface_->screen==interaction::Screen::Inventory))
            action_=Action::Cancel;
        else if (action_!=Action::Cancel) switch(key) {
        case SDL_SCANCODE_A: case SDL_SCANCODE_LEFT: action_=Action::Left; break;
        case SDL_SCANCODE_D: case SDL_SCANCODE_RIGHT: action_=Action::Right; break;
        case SDL_SCANCODE_W: case SDL_SCANCODE_UP: action_=Action::Up; break;
        case SDL_SCANCODE_S: case SDL_SCANCODE_DOWN: action_=Action::Down; break;
        case SDL_SCANCODE_E: case SDL_SCANCODE_SPACE: action_=Action::Confirm; break;
        default: break;
        }
        return true;
    }
    switch(key) {
    case SDL_SCANCODE_I: if (available_) inventoryRequested_=true; break;
    case SDL_SCANCODE_E: take_=true; break;
    case SDL_SCANCODE_Z: injector_=true; break;
    case SDL_SCANCODE_X: oxygen_=true; break;
    default: break;
    }
    return gameplayAction;
}
