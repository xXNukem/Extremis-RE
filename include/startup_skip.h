#pragma once
#include <SDL3/SDL.h>

// Host adaptation, used only by the three startup players.
inline bool startupSkipRequested(const SDL_Event& event, SDL_WindowID window)
{
    return event.type==SDL_EVENT_KEY_DOWN && event.key.windowID==window &&
           !event.key.repeat && (event.key.scancode==SDL_SCANCODE_SPACE ||
                                 event.key.scancode==SDL_SCANCODE_E);
}
inline void clearStartupSkipInput()
{
    SDL_ResetKeyboard();
    SDL_FlushEvents(SDL_EVENT_KEY_DOWN,SDL_EVENT_KEY_UP);
}
