#pragma once

#include "player_control.h"
#include "weapon_fire.h"
#include "door_control.h"
#include <array>

struct SDL_Window;
union SDL_Event;

// Owns input capture for one window; destroy before its SDL_Window.
class PlayerInput {
public:
    PlayerInput(SDL_Window* window, DoorControl& doors);
    ~PlayerInput();
    PlayerInput(const PlayerInput&) = delete;
    PlayerInput& operator=(const PlayerInput&) = delete;
    void handleEvent(const SDL_Event& event, bool gameplay = true);
    void invalidateContext() { reset_=true; firePressed_=false; fireReady_=false; }
    bool update(Camera* camera, bool view3D, double seconds);
    double movementDistance() const;
    bool running() const;
    player::FireTrigger takeFireTrigger();
    bool fireReady() const { return fireReady_; }
private:
    SDL_Window* window_;
    DoorControl& doors_;
    bool active_ = false;
    bool reset_ = false;
    double movementDistance_ = 0;
    bool running_ = false;
    bool fireHeld_ = false, firePressed_ = false, fireReady_ = false, fireSuppressed_ = false;
    std::array<bool,8> blockedKeys_{};
    player::Settings settings_;
    enum class CaptureStatus { Failed, Waiting, Ready };
    CaptureStatus updateCapture(bool desired);
};
