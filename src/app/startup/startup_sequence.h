#pragma once

#include "launch_options.h"
#include "app/startup/startup_credits.h"

#include <filesystem>

struct UserSettings;
namespace continuation { struct Checkpoint; }

namespace app {
StartupCreditsResult playStartupSequence(SDL_Window* window, SDL_Renderer* renderer,
    const LaunchOptions& options, const std::filesystem::path& dataDirectory,
    bool hasLevel, UserSettings& settings, const std::filesystem::path& settingsPath, int& exitCode,
    const continuation::Checkpoint*& checkpoint);
}
