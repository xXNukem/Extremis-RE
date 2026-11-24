#pragma once

#include <filesystem>
#include "app/startup/port_menu_types.h"
#include "app/startup/user_settings.h"
#include <functional>

struct SDL_Window;
struct SDL_Renderer;

void initializePortVideo(SDL_Window* window, SDL_Renderer* renderer, UserSettings& settings);

PortMenuResult showPortMenu(SDL_Window* window, SDL_Renderer* renderer,
                            const std::filesystem::path& dataDirectory, bool inGame,
                            UserSettings& settings, const std::filesystem::path& settingsPath,
                            const std::function<void(float)>& onVolumeChanged = {});
