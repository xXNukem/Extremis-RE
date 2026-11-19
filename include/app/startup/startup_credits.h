#pragma once

#include "extremis/cinematics/playback_result.h"

#include <filesystem>

struct SDL_Window;
struct SDL_Renderer;

StartupCreditsResult showStartupCredits(SDL_Window* window, SDL_Renderer* renderer,
                                        const std::filesystem::path& resourceDirectory = {});
