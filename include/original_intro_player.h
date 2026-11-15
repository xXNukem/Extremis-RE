#pragma once
#include "startup_credits.h"
// PSEQ narrative only. The customized Extremis RE credits remain independent.
StartupCreditsResult showOriginalIntro(SDL_Window* window, SDL_Renderer* renderer,
                                      const std::filesystem::path& resourceDirectory);

// Original Blue Sphere/title/credits block, before the independent PSEQ player.
StartupCreditsResult showOriginalPresentation(SDL_Window* window, SDL_Renderer* renderer,
                                             const std::filesystem::path& resourceDirectory);
