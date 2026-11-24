#pragma once
#include "extremis/cinematics/playback_result.h"

#include <filesystem>

struct SDL_Renderer;

struct SDL_Window;
// PSEQ narrative only. The customized Extremis RE credits remain independent.
StartupCreditsResult showOriginalIntro(SDL_Window* window, SDL_Renderer* renderer,
                                      const std::filesystem::path& resourceDirectory, float gain = 1.f);

// Original Blue Sphere/title/credits block, before the independent PSEQ player.
StartupCreditsResult showOriginalPresentation(SDL_Window* window, SDL_Renderer* renderer,
                                             const std::filesystem::path& resourceDirectory);
