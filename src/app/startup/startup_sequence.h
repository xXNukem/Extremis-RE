#pragma once

#include "launch_options.h"
#include "app/startup/startup_credits.h"

#include <filesystem>

struct PortMenuVideoState;

namespace app {
StartupCreditsResult playStartupSequence(SDL_Window* window, SDL_Renderer* renderer,
    const LaunchOptions& options, const std::filesystem::path& dataDirectory,
    bool hasLevel, PortMenuVideoState& video, int& exitCode);
}
