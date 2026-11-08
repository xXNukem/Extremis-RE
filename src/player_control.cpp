#include "player_control.h"

#include <algorithm>
#include <cmath>

namespace player {
unsigned update(Camera& camera, const Input& input, double seconds, const Settings& settings,
            const collision::World* world, bool audioEnabled, const MovementConstraint& constrain,
            const MovementConstraint& beforeWalls)
{
    if (!std::isfinite(seconds) || seconds < 0) return 0;
    constexpr double tau = 6.28318530717958647692;
    // Screen right is (sin(angle), -cos(angle)) in the existing map convention.
    camera.angle = std::remainder(camera.angle + input.turnLeft * settings.turnSpeed * seconds
                                 - input.mouseX * settings.mouseSensitivity, tau);
    const double magnitude = std::max(1.0, std::hypot(input.forward, input.right));
    const double forward = input.forward / magnitude;
    const double right = input.right / magnitude;
    // Preserve port walking/running speeds. Native step 10 at 18 Hz supplies
    // the ratio for the DOS deceleration of 2 per update, not render frame.
    constexpr double tick = 1.0 / 18;
    unsigned footsteps = 0;
    double remaining = seconds;
    while (remaining > 1e-12) {
        if (camera.postureTime == 0) {
            camera.bobStartX = camera.x;
            camera.bobStartY = camera.y;
        }
        const Camera before = camera;
        if (camera.postureHeight >= 150)
            camera.movementStep = input.run ? 10 * settings.runMultiplier : 10;
        const double dt = std::min(remaining, tick - camera.postureTime);
        const double distance = input.blockMovement ? 0 :
            settings.moveSpeed * (camera.movementStep / 10) * dt;
        const collision::Point displacement{
            (std::cos(camera.angle) * forward + std::sin(camera.angle) * right) * distance,
            (std::sin(camera.angle) * forward - std::cos(camera.angle) * right) * distance};
        Camera candidate = camera;
        candidate.x += displacement.x; candidate.y += displacement.y;
        if (beforeWalls) beforeWalls(before,candidate);
        const auto position = world ? collision::move(*world, {camera.x, camera.y},
                                                     {candidate.x-camera.x,candidate.y-camera.y},
                                                     settings.collision)
                                    : collision::Point{candidate.x,candidate.y};
        camera.x = position.x;
        camera.y = position.y;
        if (constrain) constrain(before, camera);
        remaining -= dt;
        camera.postureTime += dt;
        if (camera.postureTime >= tick - 1e-12) {
            camera.postureTime = 0;
            if (!input.blockMovement &&
                (camera.x != camera.bobStartX || camera.y != camera.bobStartY)) {
                if (input.forward == 0) camera.bobPhase = 0;
                camera.bobPhase += static_cast<unsigned>(std::floor(16 * camera.movementStep / 10));
                if (camera.bobPhase >= 144) {
                    camera.bobPhase = audioEnabled ? 28 : 7;
                    ++footsteps;
                }
            }
            camera.postureHeight = std::clamp(camera.postureHeight +
                                              (input.crouch ? -10 : 10), 100.0, 165.0);
            if (camera.postureHeight < 150)
                camera.movementStep = std::max(10.0, camera.movementStep - 2);
        }
    }
    return footsteps;
}
void recoil(Camera& camera, const collision::World& world, const collision::Settings& settings,
            const MovementConstraint& beforeWalls)
{
    // DOS 2 * min(30, floor(180/FPS)), adapted to the existing 18 Hz clock.
    Camera candidate = camera;
    candidate.x -= 20*std::cos(camera.angle); candidate.y -= 20*std::sin(camera.angle);
    if (beforeWalls) beforeWalls(camera,candidate);
    const auto position = collision::move(world, {camera.x,camera.y},
        {candidate.x-camera.x,candidate.y-camera.y},settings);
    camera.bobStartX += position.x-camera.x;
    camera.bobStartY += position.y-camera.y;
    camera.x=position.x; camera.y=position.y;
}
} // namespace player
