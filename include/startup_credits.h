#pragma once

#include <filesystem>

struct SDL_Window;
struct SDL_Renderer;

enum class StartupCreditsResult { Continue, Quit, Error };
StartupCreditsResult showStartupCredits(SDL_Window* window, SDL_Renderer* renderer,
                                        const std::filesystem::path& resourceDirectory = {});
