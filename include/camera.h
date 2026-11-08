#pragma once

struct Camera {
    static constexpr double eyeHeight = 165.0;
    static constexpr double horizontalFov = 70.0 * 3.14159265358979323846 / 180.0;
    double x = 0, y = 0;
    double angle = 0; // Radians: zero = +X, positive rotation towards +Y.
    // DOS posture, advanced at the port's adapted 18 Hz cadence.
    double postureHeight = eyeHeight;
    double postureTime = 0;
    double movementStep = 10; // DOS walking step at 18 Hz; scaled to port speed.
    double viewHeight() const { return postureHeight; }
};
