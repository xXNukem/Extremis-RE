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
    const double distance = settings.moveSpeed * (input.run?settings.runMultiplier:1.0) * seconds;
    const collision::Point displacement{
        (std::cos(camera.angle) * forward + std::sin(camera.angle) * right) * distance,
        (std::sin(camera.angle) * forward - std::cos(camera.angle) * right) * distance};
    const auto position = world ? collision::move(*world, {camera.x, camera.y}, displacement,
                                                 settings.collision)
                                : collision::Point{camera.x + displacement.x,
                                                   camera.y + displacement.y};
    camera.x = position.x;
    camera.y = position.y;
}
} // namespace player
