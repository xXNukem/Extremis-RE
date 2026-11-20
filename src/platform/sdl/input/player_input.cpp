#include "platform/sdl/input/player_input.h"

#include <SDL3/SDL.h>
#include <cmath>

PlayerInput::PlayerInput(SDL_Window* window, DoorControl& doors)
    : devices_(window, {SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_A,
          SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_LSHIFT, SDL_SCANCODE_RSHIFT,
          SDL_SCANCODE_LALT, SDL_SCANCODE_RALT}, SDL_BUTTON_LEFT),
      doors_(doors) {}

void PlayerInput::handleEvent(const SDL_Event& event, bool gameplay)
{
    devices_.handleEvent(event, gameplay);
#ifdef EXTREMIS_DEBUG_MENU
    // Even multiple TAB toggles in one event pump must discard accumulated motion.
    if (event.type == SDL_EVENT_KEY_DOWN && event.key.windowID == devices_.windowId() &&
        event.key.key == SDLK_TAB && !event.key.repeat)
        devices_.requestCaptureReset();
#endif
}

bool PlayerInput::update(Camera* camera, bool view3D, double seconds, bool deferMovement)
{
    footsteps_ = 0;
    movementDistance_ = 0;
    running_ = false;

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
    if (!devices_.ready()) return; // Capture, modal and focus gates still own input.
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
    const auto button = devices_.takeButton();
    return {button.held, button.pressed};
}

void PlayerInput::invalidateContext()
{
    devices_.invalidateContext();
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
    return devices_.ready();
}
