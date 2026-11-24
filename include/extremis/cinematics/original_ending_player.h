#pragma once
#include "extremis/cinematics/original_ending.h"
#include "extremis/cinematics/playback_result.h"

#include <filesystem>

struct SDL_Renderer;

struct SDL_Window;

// Blocking, independent presentation. Does not enter a menu or touch campaign state.
StartupCreditsResult showOriginalEnding(SDL_Window* window,SDL_Renderer* renderer,
    const std::filesystem::path& directory,original_ending::EntryImage entry={}, float gain=1.f);
