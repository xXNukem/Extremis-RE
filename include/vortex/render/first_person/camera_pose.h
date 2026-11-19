#pragma once

namespace vortex {
struct CameraPose {
    double x = 0, y = 0;
    double angle = 0; // Radians: zero = +X, positive rotation towards +Y.
};
}
