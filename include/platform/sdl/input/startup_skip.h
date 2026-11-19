#pragma once
#include <SDL3/SDL.h>

// Host adaptation, used only by the three startup players.
bool startupSkipRequested(const SDL_Event& event, SDL_WindowID window);
void clearStartupSkipInput();
