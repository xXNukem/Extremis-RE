#pragma once
#include "extremis/cinematics/playback_result.h"

#include <filesystem>

struct SDL_Renderer;

struct SDL_Window;
class GameAudio;
// Blocking host adapter. Error/Quit never authorizes committing the destination.
StartupCreditsResult showOriginalElevator(SDL_Window* window,SDL_Renderer* renderer,
    const std::filesystem::path& directory,int floorDelta,GameAudio& audio);
