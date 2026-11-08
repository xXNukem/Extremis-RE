#include "player_control.h"

#include <algorithm>
#include <cmath>

namespace player {
void update(Camera& camera, const Input& input, double seconds, const Settings& settings,
            const collision::World* world)
{
    if (!std::isfinite(seconds) || seconds < 0) return;
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
    double remaining = seconds;
    while (remaining > 1e-12) {
        if (camera.postureHeight >= 150)
            camera.movementStep = input.run ? 10 * settings.runMultiplier : 10;
        const double dt = std::min(remaining, tick - camera.postureTime);
        const double distance = input.blockMovement ? 0 :
            settings.moveSpeed * (camera.movementStep / 10) * dt;
        const collision::Point displacement{
            (std::cos(camera.angle) * forward + std::sin(camera.angle) * right) * distance,
            (std::sin(camera.angle) * forward - std::cos(camera.angle) * right) * distance};
        const auto position = world ? collision::move(*world, {camera.x, camera.y}, displacement,
                                                     settings.collision)
                                    : collision::Point{camera.x + displacement.x,
                                                       camera.y + displacement.y};
        camera.x = position.x;
        camera.y = position.y;
        remaining -= dt;
        camera.postureTime += dt;
        if (camera.postureTime >= tick - 1e-12) {
            camera.postureTime = 0;
            camera.postureHeight = std::clamp(camera.postureHeight +
                                              (input.crouch ? -10 : 10), 100.0, 165.0);
            if (camera.postureHeight < 150)
                camera.movementStep = std::max(10.0, camera.movementStep - 2);
        }
    }
}
void recoil(Camera& camera, const collision::World& world, const collision::Settings& settings)
{
    // DOS 2 * min(30, floor(180/FPS)), adapted to the existing 18 Hz clock.
    const auto position = collision::move(world, {camera.x,camera.y},
        {-20*std::cos(camera.angle),-20*std::sin(camera.angle)},settings);
    camera.x=position.x; camera.y=position.y;
}
} // namespace player
