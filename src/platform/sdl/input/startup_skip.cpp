#include "platform/sdl/input/startup_skip.h"

bool startupSkipRequested(const SDL_Event& event, SDL_WindowID window)
{
    return event.type==SDL_EVENT_KEY_DOWN && event.key.windowID==window &&
           !event.key.repeat && (event.key.scancode==SDL_SCANCODE_SPACE ||
                                 event.key.scancode==SDL_SCANCODE_E);
}

void clearStartupSkipInput()
{
    SDL_ResetKeyboard();
    SDL_FlushEvents(SDL_EVENT_KEY_DOWN,SDL_EVENT_KEY_UP);
}
