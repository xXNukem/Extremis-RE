#pragma once

#include "extremis/gameplay/player/player_control.h"
#include "extremis/gameplay/player/weapon_fire.h"
#include "extremis/world/door_control.h"
#include "vortex/input/sdl_input.h"

struct SDL_Window;
union SDL_Event;

// Owns input capture for one window; destroy before its SDL_Window.
class PlayerInput {
public:
    PlayerInput(SDL_Window* window, DoorControl& doors);
    PlayerInput(const PlayerInput&) = delete;
    PlayerInput& operator=(const PlayerInput&) = delete;
    void handleEvent(const SDL_Event& event, bool gameplay = true);
    void invalidateContext();
    void releaseRelativeMouse();
    bool update(Camera* camera, bool view3D, double seconds, bool deferMovement = false);
    void move(Camera& camera, double seconds, bool blocked, const player::MovementConstraint& constrain = {});
    void setAudioEnabled(bool enabled);
    void setObjectConstraint(const player::MovementConstraint& constraint);
    unsigned footsteps() const;
    void recoil(Camera& camera);
    double movementDistance() const;
    bool running() const;
    player::FireTrigger takeFireTrigger();
    bool fireReady() const;
private:
    vortex::input::SDLInput devices_;
    DoorControl& doors_;
    double movementDistance_ = 0;
    unsigned footsteps_ = 0;
    bool audioEnabled_ = false;
    bool running_ = false;
    player::Settings settings_;
    player::Input input_;
    player::MovementConstraint objectConstraint_;
};
