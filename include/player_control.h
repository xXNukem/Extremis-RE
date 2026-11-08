#pragma once

#include "camera.h"
#include "player_collision.h"

namespace player {
struct Settings {
    double moveSpeed = 300.0; // Provisional world units / second.
    double runMultiplier = 2.0; // DOS maximum speed is twice the walking step.
    double turnSpeed = 1.5; // Existing arrow rotation, radians / second.
    double mouseSensitivity = 0.003; // Provisional radians / relative mouse unit.
    collision::Settings collision;
};
struct Input {
    double forward = 0;
    double right = 0;
    double turnLeft = 0;
    double mouseX = 0;
    bool run = false;
    bool crouch = false;
    bool blockMovement = false; // Reaction suppresses translation, not turning/posture.
};

// Planar movement and fixed-step camera posture without SDL; optionally resolves static collisions.
void update(Camera& camera, const Input& input, double seconds,
            const Settings& settings = {}, const collision::World* world = nullptr);
// One reaction tick; duration and damage remain owned by enemies::System.
void recoil(Camera& camera, const collision::World& world,
            const collision::Settings& settings = {});
} // namespace player
