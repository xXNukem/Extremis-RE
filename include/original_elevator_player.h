#pragma once
#include "startup_credits.h"
class GameAudio;
// Blocking host adapter. Error/Quit never authorizes committing the destination.
StartupCreditsResult showOriginalElevator(SDL_Window* window,SDL_Renderer* renderer,
    const std::filesystem::path& directory,int floorDelta,GameAudio& audio);
