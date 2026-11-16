#pragma once
#include "original_ending.h"
#include "startup_credits.h"

// Blocking, independent presentation. Does not enter a menu or touch campaign state.
StartupCreditsResult showOriginalEnding(SDL_Window* window,SDL_Renderer* renderer,
    const std::filesystem::path& directory,original_ending::EntryImage entry={});
