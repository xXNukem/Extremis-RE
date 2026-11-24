#include "platform/sdl/input/player_input.h"

#include <SDL3/SDL.h>
#include <cmath>
#include <utility>

PlayerInput::PlayerInput(SDL_Window* window, DoorControl& doors)
    : window_(window),
      devices_(window, {SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_A,
          SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_LSHIFT, SDL_SCANCODE_RSHIFT,
          SDL_SCANCODE_LALT, SDL_SCANCODE_RALT}, SDL_BUTTON_LEFT),
      doors_(doors) {}

bool PlayerInput::handleEvent(const SDL_Event& event, bool gameplay)
{
    if (classicControls_) {
        if ((event.type == SDL_EVENT_WINDOW_FOCUS_GAINED || event.type == SDL_EVENT_WINDOW_FOCUS_LOST) &&
            event.window.windowID == devices_.windowId())
            invalidateContext();
        if ((event.type != SDL_EVENT_KEY_DOWN && event.type != SDL_EVENT_KEY_UP) ||
            event.key.windowID != devices_.windowId() || event.key.repeat ||
            event.key.scancode <= SDL_SCANCODE_UNKNOWN || event.key.scancode >= SDL_SCANCODE_COUNT)
            return false;

        const bool wasFiring = classicFireHeld();
        auto& held = classicKeys_[event.key.scancode];
        if (event.type == SDL_EVENT_KEY_UP) held = false;
        else if (gameplay && classicReady_) held = true;
        if (!classicFireHeld()) classicFireSuppressed_ = false;
        if (!wasFiring && classicFireHeld()) {
            classicFirePressed_ = true;
            return true;
        }
        return false;
    }

    devices_.handleEvent(event, gameplay);
#ifdef EXTREMIS_DEBUG_MENU
    // Even multiple TAB toggles in one event pump must discard accumulated motion.
    if (event.type == SDL_EVENT_KEY_DOWN && event.key.windowID == devices_.windowId() &&
        event.key.key == SDLK_TAB && !event.key.repeat)
        devices_.requestCaptureReset();
#endif
    return false;
}

void PlayerInput::setClassicControls(bool classic)
{
    if (classicControls_ == classic) return;
    if (classic && SDL_GetWindowRelativeMouseMode(window_)) releaseRelativeMouse();
    classicControls_ = classic;
    invalidateContext();
}

bool PlayerInput::classicFireHeld() const
{
    // macOS can reserve Ctrl+Up for Mission Control.
    return classicKeys_[SDL_SCANCODE_RETURN] ||
        (classicKeys_[SDL_SCANCODE_LCTRL] && classicKeys_[SDL_SCANCODE_UP]);
}

void PlayerInput::suppressClassicFire()
{
    classicFireSuppressed_ = true;
    classicFirePressed_ = false;
}

bool PlayerInput::update(Camera* camera, bool view3D, double seconds, bool deferMovement)
{
    footsteps_ = 0;
    movementDistance_ = 0;
    running_ = false;

    if (classicControls_) {
        if (!camera || !view3D || SDL_GetKeyboardFocus() != window_) {
            invalidateContext();
            return true;
        }
        classicReady_ = true;
        input_ = {
            classicKeys_[SDL_SCANCODE_DOWN] ? -1.0 : double(classicKeys_[SDL_SCANCODE_UP]),
            0,
            classicKeys_[SDL_SCANCODE_RIGHT] ? -1.0 : double(classicKeys_[SDL_SCANCODE_LEFT]),
            0,
            classicKeys_[SDL_SCANCODE_LALT],
            classicKeys_[SDL_SCANCODE_SPACE]};
        if (!deferMovement) move(*camera,seconds,false);
        return true;
    }

    using CaptureStatus = vortex::input::SDLInput::CaptureStatus;
    const auto capture = devices_.updateCapture(camera && view3D);
    if (capture != CaptureStatus::Ready) return capture != CaptureStatus::Failed;

    float mouseX = 0;
    devices_.takeRelativeMouse(&mouseX, nullptr);
    devices_.sampleKeyboard();
    input_ = {
        double(devices_.keyDown(SDL_SCANCODE_W)) - double(devices_.keyDown(SDL_SCANCODE_S)),
        double(devices_.keyDown(SDL_SCANCODE_D)) - double(devices_.keyDown(SDL_SCANCODE_A)),
        double(devices_.keyDown(SDL_SCANCODE_LEFT)) - double(devices_.keyDown(SDL_SCANCODE_RIGHT)),
        mouseX,
        devices_.keyDown(SDL_SCANCODE_LSHIFT) || devices_.keyDown(SDL_SCANCODE_RSHIFT),
        devices_.keyDown(SDL_SCANCODE_LALT) || devices_.keyDown(SDL_SCANCODE_RALT)};
    if (!deferMovement) move(*camera,seconds,false);
    return true;
}

void PlayerInput::move(Camera& camera, double seconds, bool blocked, const player::MovementConstraint& constrain)
{
    if (!fireReady()) return;
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
    if (classicControls_) {
        const bool pressed = std::exchange(classicFirePressed_, false);
        return {classicReady_ && !classicFireSuppressed_ && classicFireHeld(), classicReady_ && pressed};
    }
    const auto button = devices_.takeButton();
    return {button.held, button.pressed};
}

void PlayerInput::invalidateContext()
{
    devices_.invalidateContext();
    classicKeys_.fill(false);
    classicReady_ = classicFirePressed_ = classicFireSuppressed_ = false;
}

void PlayerInput::releaseRelativeMouse()
{
    devices_.releaseRelativeMouse();
}

void PlayerInput::setAudioEnabled(bool enabled)
{
    audioEnabled_ = enabled;
}

void PlayerInput::setObjectConstraint(const player::MovementConstraint& constraint)
{
    objectConstraint_ = constraint;
}

unsigned PlayerInput::footsteps() const
{
    return footsteps_;
}

bool PlayerInput::fireReady() const
{
    return classicControls_ ? classicReady_ : devices_.ready();
}
