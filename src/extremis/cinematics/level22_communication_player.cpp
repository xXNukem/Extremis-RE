#include "extremis/cinematics/level22_communication.h"
#include <SDL3/SDL.h>
#include <array>
#include <exception>

namespace communication {
StartupCreditsResult show(SDL_Window* window,const std::filesystem::path& directory,
    const std::function<bool(const std::vector<std::uint32_t>&)>& present) {
    struct ClearInput {
        ~ClearInput() {SDL_ResetKeyboard();SDL_FlushEvents(SDL_EVENT_KEY_DOWN,SDL_EVENT_KEY_UP);}
    } clear;
    try {
        Presentation presentation(loadResources(directory));
        std::array<bool,SDL_SCANCODE_COUNT> held{};
        const auto* keyboard=SDL_GetKeyboardState(nullptr);
        bool blockAction=keyboard[SDL_SCANCODE_E] || keyboard[SDL_SCANCODE_SPACE];
        bool focused=SDL_GetKeyboardFocus()==window;
        const auto id=SDL_GetWindowID(window);auto previous=SDL_GetTicksNS();
        SDL_Log("Level22 communication: nominal PIT/70 Hz presentation; DOS timing comparison pending");
        while(presentation.phase()!=Phase::Done) {
            bool pressed=false;int choice=0;
            SDL_Event event;
            while(SDL_PollEvent(&event)) {
                if(event.type==SDL_EVENT_QUIT || (event.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID==id))
                    return StartupCreditsResult::Quit;
                if(event.type==SDL_EVENT_WINDOW_FOCUS_LOST && event.window.windowID==id) {
                    focused=false;held.fill(false);blockAction=true;
                }
                if(event.type==SDL_EVENT_WINDOW_FOCUS_GAINED && event.window.windowID==id) focused=true;
                if((event.type==SDL_EVENT_KEY_DOWN || event.type==SDL_EVENT_KEY_UP) && event.key.windowID==id) {
                    const auto k=event.key.scancode;
                    if(event.type==SDL_EVENT_KEY_DOWN && !event.key.repeat && focused) {
                        if(k==SDL_SCANCODE_UP || k==SDL_SCANCODE_W) choice=1;
                        if(k==SDL_SCANCODE_DOWN || k==SDL_SCANCODE_S) choice=-1;
                        if(!blockAction && (k==SDL_SCANCODE_E || k==SDL_SCANCODE_SPACE) &&
                           (presentation.phase()==Phase::Choice || presentation.phase()==Phase::Acknowledge)) pressed=true;
                    }
                    if(k>SDL_SCANCODE_UNKNOWN && k<SDL_SCANCODE_COUNT)
                        held[k]=focused && event.type==SDL_EVENT_KEY_DOWN;
                    if(event.type==SDL_EVENT_KEY_UP && (k==SDL_SCANCODE_E || k==SDL_SCANCODE_SPACE)) blockAction=false;
                }
            }
            Input input;
            input.choice=choice?choice:(held[SDL_SCANCODE_UP] || held[SDL_SCANCODE_W])?1:
                         (held[SDL_SCANCODE_DOWN] || held[SDL_SCANCODE_S])?-1:0;
            input.action=pressed || (!blockAction && (held[SDL_SCANCODE_E] || held[SDL_SCANCODE_SPACE]));
            input.escape=held[SDL_SCANCODE_ESCAPE];
            const auto now=SDL_GetTicksNS();
            if(focused) presentation.advance(double(now-previous)/1e9,input);
            previous=now;
            if(!present(presentation.pixels())) return StartupCreditsResult::Error;
            SDL_Delay(1);
        }
        return StartupCreditsResult::Continue;
    } catch(const std::exception& error) {
        SDL_Log("Level22 communication failed: %s",error.what());return StartupCreditsResult::Error;
    }
}
}
