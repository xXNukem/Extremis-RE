#pragma once

#include "vortex/core/math/constants.h"
#include "vortex/render/first_person/camera_pose.h"

#include <optional>

struct Camera {
    static constexpr double eyeHeight = 165.0;
    static constexpr double horizontalFov = 70.0 * vortex::math::pi / 180.0;
    double x = 0, y = 0;
    double angle = 0; // Radians: zero = +X, positive rotation towards +Y.
    // DOS posture, advanced at the port's adapted 18 Hz cadence.
    double postureHeight = eyeHeight;
    double postureTime = 0;
    double movementStep = 10; // DOS walking step at 18 Hz; scaled to port speed.
    unsigned bobPhase = 0;
    double bobStartX = 0, bobStartY = 0;
    std::optional<unsigned> sector; // Authoritative after spawn / connection crossing.
    vortex::CameraPose renderPose() const;
    double viewHeight() const;
};
