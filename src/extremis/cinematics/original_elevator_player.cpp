#include "extremis/cinematics/original_elevator_player.h"
#include "extremis/cinematics/original_elevator.h"
#include "extremis/audio/game_audio.h"
#include <SDL3/SDL.h>
#include <memory>
#include <stdexcept>

namespace {
struct Restore {
    SDL_Renderer* renderer;
    GameAudio& audio;
    int width=0,height=0;
    float x=1,y=1;
    SDL_RendererLogicalPresentation mode=SDL_LOGICAL_PRESENTATION_DISABLED;
    Uint8 r=0,g=0,b=0,a=255;
    Restore(SDL_Renderer* target,GameAudio& sound):renderer(target),audio(sound) {
        if(!SDL_GetRenderLogicalPresentation(renderer,&width,&height,&mode) ||
           !SDL_GetRenderScale(renderer,&x,&y) || !SDL_GetRenderDrawColor(renderer,&r,&g,&b,&a))
            throw std::runtime_error(SDL_GetError());
    }
    ~Restore() {
        audio.stopEffects();SDL_ResetKeyboard();
        SDL_SetRenderLogicalPresentation(renderer,width,height,mode);
        SDL_SetRenderScale(renderer,x,y);SDL_SetRenderDrawColor(renderer,r,g,b,a);
    }
};
}
StartupCreditsResult showOriginalElevator(SDL_Window* window,SDL_Renderer* renderer,
    const std::filesystem::path& directory,int delta,GameAudio& audio) {
    try {
        original_elevator::Presentation presentation(original_elevator::loadResources(directory),delta);
        Restore restore(renderer,audio);
        std::unique_ptr<SDL_Texture,decltype(&SDL_DestroyTexture)> texture(
            SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_STREAMING,320,200),SDL_DestroyTexture);
        if(!texture || !SDL_SetTextureScaleMode(texture.get(),SDL_SCALEMODE_NEAREST) ||
           !SDL_SetRenderLogicalPresentation(renderer,320,200,SDL_LOGICAL_PRESENTATION_LETTERBOX) ||
           !SDL_SetRenderScale(renderer,1,1)) throw std::runtime_error(SDL_GetError());
        audio.stopEffects();
        const auto id=SDL_GetWindowID(window);
        bool focused=SDL_GetKeyboardFocus()==window,motor=false;
        auto next=SDL_GetTicksNS();
        while(presentation.phase()!=original_elevator::Phase::Done) {
            SDL_Event event;
            while(SDL_PollEvent(&event)) {
                if(event.type==SDL_EVENT_QUIT || (event.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID==id))
                    return StartupCreditsResult::Quit;
                // Consume all keyboard/mouse events. Escape cannot skip or commit travel.
                if(event.type==SDL_EVENT_WINDOW_FOCUS_LOST && event.window.windowID==id) {
                    focused=false;audio.stopEffects();motor=false;
                }
                if(event.type==SDL_EVENT_WINDOW_FOCUS_GAINED && event.window.windowID==id) {
                    focused=true;next=SDL_GetTicksNS();
                }
            }
            const auto now=SDL_GetTicksNS();
            if(!focused || now<next) {SDL_Delay(1);continue;}
            presentation.step();
            if(presentation.motor()!=motor) {
                motor=presentation.motor();
                if(motor) audio.elevatorMotor(); else audio.stopEffects();
            }
            if(!SDL_UpdateTexture(texture.get(),nullptr,presentation.pixels().data(),320*sizeof(std::uint32_t)) ||
               !SDL_SetRenderDrawColor(renderer,0,0,0,255) || !SDL_RenderClear(renderer) ||
               !SDL_RenderTexture(renderer,texture.get(),nullptr,nullptr) || !SDL_RenderPresent(renderer))
                throw std::runtime_error(SDL_GetError());
            // Host adaptation: nominal 70 Hz retrace, never skip procedural draws.
            next=now+1000000000/70;
        }
        return StartupCreditsResult::Continue;
    } catch(const std::exception& error) {
        SDL_Log("Original elevator failed: %s",error.what());
        return StartupCreditsResult::Error;
    }
}
