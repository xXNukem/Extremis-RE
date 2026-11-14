#include "player_input.h"

#include <SDL3/SDL.h>
#include <cmath>
#include <utility>

namespace {
constexpr std::array<SDL_Scancode,10> movementKeys{SDL_SCANCODE_W,SDL_SCANCODE_S,
    SDL_SCANCODE_D,SDL_SCANCODE_A,SDL_SCANCODE_LEFT,SDL_SCANCODE_RIGHT,
    SDL_SCANCODE_LSHIFT,SDL_SCANCODE_RSHIFT,SDL_SCANCODE_LALT,SDL_SCANCODE_RALT};
}

PlayerInput::PlayerInput(SDL_Window* window, DoorControl& doors)
    : window_(window), doors_(doors) {}

PlayerInput::~PlayerInput()
{
    if (active_) SDL_SetWindowRelativeMouseMode(window_, false);
}

void PlayerInput::handleEvent(const SDL_Event& event, bool gameplay)
{
    if ((event.type==SDL_EVENT_MOUSE_BUTTON_DOWN || event.type==SDL_EVENT_MOUSE_BUTTON_UP) &&
        event.button.windowID==SDL_GetWindowID(window_) && event.button.button==SDL_BUTTON_LEFT) {
        if (event.type==SDL_EVENT_MOUSE_BUTTON_DOWN) {
            if (!gameplay || !active_ || reset_) { fireSuppressed_=true; firePressed_=false; }
            else if (!fireHeld_ && !fireSuppressed_) firePressed_=true;
            fireHeld_=true;
        } else { fireHeld_=false; fireSuppressed_=false; }
    }
    if (event.type==SDL_EVENT_KEY_UP && event.key.windowID==SDL_GetWindowID(window_))
        for (unsigned i=0;i<movementKeys.size();++i)
            if (event.key.scancode==movementKeys[i]) blockedKeys_[i]=false;
    if ((event.type == SDL_EVENT_WINDOW_FOCUS_GAINED ||
         event.type == SDL_EVENT_WINDOW_FOCUS_LOST) &&
        event.window.windowID == SDL_GetWindowID(window_)) {
        reset_=true; firePressed_=false; fireReady_=false;
    }
#ifdef EXTREMIS_DEBUG_MENU
    // Even multiple TAB toggles in one event pump must discard accumulated motion.
    if (event.type == SDL_EVENT_KEY_DOWN && event.key.windowID == SDL_GetWindowID(window_) &&
        event.key.key == SDLK_TAB && !event.key.repeat) reset_ = true;
#endif
}

PlayerInput::CaptureStatus PlayerInput::updateCapture(bool desired)
{
    if (desired != active_ || reset_) {
        if (!SDL_SetWindowRelativeMouseMode(window_, desired)) return CaptureStatus::Failed;
        active_ = desired;
        firePressed_=false;
        const auto buttons=SDL_GetMouseState(nullptr,nullptr)|SDL_GetGlobalMouseState(nullptr,nullptr);
        fireHeld_=(buttons&SDL_BUTTON_LMASK)!=0;
        fireSuppressed_=fireHeld_;
        const bool* keys=SDL_GetKeyboardState(nullptr);
        for (unsigned i=0;i<movementKeys.size();++i) blockedKeys_[i]=desired && keys[movementKeys[i]];
        reset_ = false;
        SDL_GetRelativeMouseState(nullptr, nullptr);
        return CaptureStatus::Waiting; // Discard capture/context transitions and require held modal keys to be released.
    }
    if (!active_) {
        SDL_GetRelativeMouseState(nullptr, nullptr);
        return CaptureStatus::Waiting;
    }
    return CaptureStatus::Ready;
}

bool PlayerInput::update(Camera* camera, bool view3D, double seconds, bool deferMovement)
{
    footsteps_ = 0;
    movementDistance_ = 0; running_=false; fireReady_=false;

    const bool desired = camera && view3D && SDL_GetKeyboardFocus() == window_;
    const auto capture = updateCapture(desired);
    if (capture != CaptureStatus::Ready) { firePressed_=false; return capture != CaptureStatus::Failed; }
    fireReady_=true;
    float mouseX = 0;
    SDL_GetRelativeMouseState(&mouseX, nullptr);
    const bool* keys = SDL_GetKeyboardState(nullptr);
    std::array<bool,10> pressed{};
    for (unsigned i=0;i<movementKeys.size();++i) {
        if (!keys[movementKeys[i]]) blockedKeys_[i]=false;
        pressed[i]=keys[movementKeys[i]] && !blockedKeys_[i];
    }
    input_={
        double(pressed[0])-double(pressed[1]), double(pressed[2])-double(pressed[3]),
        double(pressed[4])-double(pressed[5]),mouseX,pressed[6] || pressed[7],pressed[8] || pressed[9]};
    if (!deferMovement) move(*camera,seconds,false);
    return true;
}

void PlayerInput::move(Camera& camera, double seconds, bool blocked, const player::MovementConstraint& constrain)
{
    if (!fireReady_) return; // Capture, modal and focus gates still own input.
    input_.blockMovement=blocked;
    const double beforeX=camera.x, beforeY=camera.y;
    footsteps_ += player::update(camera,input_,seconds,settings_,&doors_.staticWorld(),audioEnabled_,constrain,objectConstraint_);
    input_.mouseX=0; // Relative mouse motion is consumed once, even across ticks.
    running_=!blocked && input_.run && camera.postureHeight>=150 &&
        (input_.forward!=0 || input_.right!=0);
    movementDistance_+=std::hypot(camera.x-beforeX,camera.y-beforeY);
}

void PlayerInput::recoil(Camera& camera)
{
    player::recoil(camera,doors_.staticWorld(),settings_.collision,objectConstraint_);
    running_=false;
}

double PlayerInput::movementDistance() const { return movementDistance_; }

bool PlayerInput::running() const { return running_; }

player::FireTrigger PlayerInput::takeFireTrigger()
{
    const bool pressed=std::exchange(firePressed_,false);
    return {fireReady_ && !fireSuppressed_ && fireHeld_,fireReady_ && !fireSuppressed_ && pressed};
}
