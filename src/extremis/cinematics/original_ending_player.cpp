#include "extremis/cinematics/original_ending_player.h"
#include "platform/sdl/audio/cinematic_voice_stream.h"
#include "platform/sdl/video/render_state.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>
#include <utility>

namespace {
audio::Bank loadVoices(const std::filesystem::path& directory)
{
    audio::Bank bank(1); // Index zero is the stop sentinel.
    using namespace original_ending;
    for (unsigned i = 1; i < static_cast<unsigned>(Sound::Count); ++i)
        bank.push_back({audio::loadSample(directory / soundFile(static_cast<Sound>(i))), 64, 0, 0});
    return bank;
}
} // namespace

StartupCreditsResult showOriginalEnding(SDL_Window* window,SDL_Renderer* renderer,
    const std::filesystem::path& directory,original_ending::EntryImage entry,float gain) {
    try {
        original_ending::Presentation ending(original_ending::loadResources(directory),std::move(entry));
        platform::sdl::RenderState restore(renderer);
        if(!SDL_SetRenderLogicalPresentation(renderer,320,200,SDL_LOGICAL_PRESENTATION_LETTERBOX) ||
           !SDL_SetRenderScale(renderer,1,1)) throw std::runtime_error(SDL_GetError());
        std::unique_ptr<SDL_Texture,decltype(&SDL_DestroyTexture)> texture(
            SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_STREAMING,320,200),SDL_DestroyTexture);
        if(!texture || !SDL_SetTextureScaleMode(texture.get(),SDL_SCALEMODE_NEAREST)) throw std::runtime_error(SDL_GetError());
        platform::sdl::CinematicVoiceStream audio;
        audio.open(loadVoices(directory), "Original ending", gain);
        SDL_Log("Original ending: nominal PIT/70 Hz retrace timing; Escape disabled (see ending.md)");
        bool focused=(SDL_GetWindowFlags(window)&SDL_WINDOW_INPUT_FOCUS)!=0;
        audio.pause(!focused);
        const auto id=SDL_GetWindowID(window);
        auto previous=SDL_GetTicksNS();
        while(ending.scene()!=original_ending::Scene::Done) {
            SDL_Event event;
            while(SDL_PollEvent(&event)) {
                if(event.type==SDL_EVENT_QUIT || (event.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID==id))
                    return StartupCreditsResult::Quit;
                // No key can skip/advance FSEQ, including Escape.
                if(event.type==SDL_EVENT_WINDOW_FOCUS_LOST && event.window.windowID==id) {focused=false;audio.pause(true);}
                if(event.type==SDL_EVENT_WINDOW_FOCUS_GAINED && event.window.windowID==id) {focused=true;audio.pause(false);}
            }
            const auto now=SDL_GetTicksNS();
            if(focused) ending.advance(double(now-previous)/1e9);
            previous=now;audio.cues(ending.takeCues());
            if(!SDL_UpdateTexture(texture.get(),nullptr,ending.pixels().data(),320*sizeof(std::uint32_t)) ||
               !SDL_SetRenderDrawColor(renderer,0,0,0,255) || !SDL_RenderClear(renderer) ||
               !SDL_RenderTexture(renderer,texture.get(),nullptr,nullptr) || !SDL_RenderPresent(renderer))
                throw std::runtime_error(SDL_GetError());
            SDL_Delay(1);
        }
        SDL_ResetKeyboard(); // Explicit host boundary; no pending keys leak to a future caller.
        return StartupCreditsResult::Continue;
    } catch(const std::exception& e) {
        SDL_Log("Original ending failed: %s",e.what());
        return StartupCreditsResult::Error;
    }
}
