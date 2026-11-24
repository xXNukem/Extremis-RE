#include <cstdint>
#include <filesystem>
#include "extremis/cinematics/original_intro_player.h"
#include "platform/sdl/input/startup_skip.h"
#include "extremis/cinematics/original_intro.h"
#include "extremis/cinematics/original_presentation.h"
#include "platform/sdl/audio/cinematic_voice_stream.h"
#include "platform/sdl/video/render_state.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>

namespace {
audio::Bank loadVoices(const std::filesystem::path& directory)
{
    audio::Bank bank(1); // Index zero is the stop sentinel.
    using namespace original_intro;
    for (unsigned i = 1; i < static_cast<unsigned>(Sound::Count); ++i)
        bank.push_back({audio::loadSample(directory / soundFile(static_cast<Sound>(i))), 64, 0, 0});
    return bank;
}
} // namespace

StartupCreditsResult showOriginalIntro(SDL_Window* window,SDL_Renderer* renderer,
                                      const std::filesystem::path& directory, float gain) {
    try {
        bool skipAtEntry=false;
        SDL_Event initialEvent;
        while(SDL_PollEvent(&initialEvent)) {
            if(initialEvent.type==SDL_EVENT_QUIT || (initialEvent.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED && initialEvent.window.windowID==SDL_GetWindowID(window)))
                return StartupCreditsResult::Quit;
            if(startupSkipRequested(initialEvent,SDL_GetWindowID(window))) {
                clearStartupSkipInput();
                return StartupCreditsResult::SkipAll;
            }
            if(initialEvent.type==SDL_EVENT_KEY_DOWN && initialEvent.window.windowID==SDL_GetWindowID(window) && !initialEvent.key.repeat && initialEvent.key.scancode==SDL_SCANCODE_ESCAPE)
                skipAtEntry=true;
        }
        original_intro::Presentation intro(original_intro::loadResources(directory),skipAtEntry);
        SDL_Log("Original intro: PSEQ12 invalid-buffer sprite draw suppressed; host loop/RNG timing adapted (see introduction.md)");
        platform::sdl::RenderState restore(renderer);
        if(!SDL_SetRenderLogicalPresentation(renderer,320,200,SDL_LOGICAL_PRESENTATION_LETTERBOX) ||
           !SDL_SetRenderScale(renderer,1,1)) throw std::runtime_error(SDL_GetError());
        std::unique_ptr<SDL_Texture,decltype(&SDL_DestroyTexture)> texture(
            SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_STREAMING,320,200),SDL_DestroyTexture);
        if(!texture || !SDL_SetTextureScaleMode(texture.get(),SDL_SCALEMODE_NEAREST)) throw std::runtime_error(SDL_GetError());
        platform::sdl::CinematicVoiceStream audio;
        try {
            audio.open(loadVoices(directory), "Original intro", gain);
        } catch (const std::exception& error) {
            SDL_Log("Original intro audio unavailable: %s", error.what());
        }
        audio.cues(intro.takeCues());
        if(skipAtEntry) { SDL_ResetKeyboard(); return StartupCreditsResult::Continue; }
        bool focused=(SDL_GetWindowFlags(window)&SDL_WINDOW_INPUT_FOCUS)!=0;
        audio.pause(!focused);
        const auto id=SDL_GetWindowID(window);
        auto previous=SDL_GetTicksNS();
        double hostPulse=0;
        bool escape=false;
        while(intro.scene()!=original_intro::Scene::Done) {
            SDL_Event event;
            while(SDL_PollEvent(&event)) {
                if(event.type==SDL_EVENT_QUIT || (event.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID==id))
                    return StartupCreditsResult::Quit;
                if(startupSkipRequested(event,id)) {
                    clearStartupSkipInput();
                    return StartupCreditsResult::SkipAll;
                }
                if(event.type==SDL_EVENT_KEY_DOWN && event.key.windowID==id && !event.key.repeat && event.key.scancode==SDL_SCANCODE_ESCAPE)
                    escape=true;
                if(event.type==SDL_EVENT_WINDOW_FOCUS_LOST && event.window.windowID==id) {focused=false;audio.pause(true);}
                if(event.type==SDL_EVENT_WINDOW_FOCUS_GAINED && event.window.windowID==id) {focused=true;audio.pause(false);}
            }
            const auto now=SDL_GetTicksNS();
            if(focused) hostPulse+=double(now-previous)/1e9;
            previous=now;
            // Adapted 60 Hz host pulses for unpaced compositions; distinct from
            // the controller's nominal PIT clock and 70 Hz retrace gate.
            if(focused && hostPulse>=1.0/(intro.unpaced()?60:70)) {
                intro.advance(hostPulse,escape,audio.voice0Active());hostPulse=0;escape=false;
                audio.cues(intro.takeCues());
            }
            if(!SDL_UpdateTexture(texture.get(),nullptr,intro.pixels().data(),320*sizeof(std::uint32_t)) ||
               !SDL_SetRenderDrawColor(renderer,0,0,0,255) || !SDL_RenderClear(renderer) ||
               !SDL_RenderTexture(renderer,texture.get(),nullptr,nullptr) || !SDL_RenderPresent(renderer))
                throw std::runtime_error(SDL_GetError());
            SDL_Delay(1);
        }
        SDL_ResetKeyboard(); // Escape may not leak into gameplay input.
        return StartupCreditsResult::Continue;
    } catch(const std::exception& e) {
        SDL_Log("Original intro failed: %s",e.what());
        return StartupCreditsResult::Error;
    }
}

// The pre-PSEQ block uses the same SDL texture/renderer-state infrastructure,
// but owns no audio and never changes the PSEQ controller.
StartupCreditsResult showOriginalPresentation(SDL_Window* window,SDL_Renderer* renderer,
                                             const std::filesystem::path& directory) {
    try {
        original_presentation::Presentation presentation(original_presentation::loadResources(directory));
        platform::sdl::RenderState restore(renderer);
        if(!SDL_SetRenderLogicalPresentation(renderer,320,200,SDL_LOGICAL_PRESENTATION_LETTERBOX) ||
           !SDL_SetRenderScale(renderer,1,1)) throw std::runtime_error(SDL_GetError());
        std::unique_ptr<SDL_Texture,decltype(&SDL_DestroyTexture)> texture(
            SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_STREAMING,320,200),SDL_DestroyTexture);
        if(!texture || !SDL_SetTextureScaleMode(texture.get(),SDL_SCALEMODE_NEAREST)) throw std::runtime_error(SDL_GetError());
        bool focused=(SDL_GetWindowFlags(window)&SDL_WINDOW_INPUT_FOCUS)!=0;
        const auto id=SDL_GetWindowID(window);
        auto previous=SDL_GetTicksNS();
        while(presentation.screen()!=original_presentation::Screen::Done) {
            bool key=false;
            SDL_Event event;
            while(SDL_PollEvent(&event)) {
                if(event.type==SDL_EVENT_QUIT || (event.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID==id))
                    return StartupCreditsResult::Quit;
                if(startupSkipRequested(event,id)) {
                    clearStartupSkipInput();
                    return StartupCreditsResult::SkipAll;
                }
                if(event.type==SDL_EVENT_KEY_DOWN && event.key.windowID==id && !event.key.repeat)
                    key=true; // Escape also advances a still card, never the whole block.
                if(event.type==SDL_EVENT_WINDOW_FOCUS_LOST && event.window.windowID==id) focused=false;
                if(event.type==SDL_EVENT_WINDOW_FOCUS_GAINED && event.window.windowID==id) focused=true;
            }
            const auto now=SDL_GetTicksNS();
            presentation.advance(focused?double(now-previous)/1e9:0,key);
            previous=now;
            if(!SDL_UpdateTexture(texture.get(),nullptr,presentation.pixels().data(),320*sizeof(std::uint32_t)) ||
               !SDL_SetRenderDrawColor(renderer,0,0,0,255) || !SDL_RenderClear(renderer) ||
               !SDL_RenderTexture(renderer,texture.get(),nullptr,nullptr) || !SDL_RenderPresent(renderer))
                throw std::runtime_error(SDL_GetError());
            SDL_Delay(1);
        }
        // Host boundary adaptation: consumed prelude keys do not skip PSEQ.
        SDL_ResetKeyboard();
        return StartupCreditsResult::Continue;
    } catch(const std::exception& e) {
        SDL_Log("Original startup presentation failed: %s",e.what());
        return StartupCreditsResult::Error;
    }
}
