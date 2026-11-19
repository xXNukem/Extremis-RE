#pragma once

#include <optional>

namespace app {
struct LaunchOptions {
    const char* labPath;
    bool communicationDebug;
    bool shuttleDebug;
    bool vitalsDebug;
    bool destructionDebug;
};

std::optional<LaunchOptions> parseLaunchOptions(int argc, char* argv[]);
}
