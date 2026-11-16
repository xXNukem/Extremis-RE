#include "original_ending_player.h"
#include "audio_mixer.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>
#include <utility>

namespace {
// Two explicitly owned DOS voices, using the existing DSS reader and mixer.
// Ending-only ownership avoids altering gameplay/death sound behavior.
class EndingAudio {
public:
    explicit EndingAudio(const std::filesystem::path& directory)
        : mixers_{audio::Mixer(banks_[0]),audio::Mixer(banks_[1])} {
        {
            audio::Bank bank(1); // index zero is the stop sentinel, never played
            for(unsigned i=1;i<static_cast<unsigned>(original_ending::Sound::Count);++i)
                bank.push_back({audio::loadSample(directory/original_ending::soundFile(static_cast<original_ending::Sound>(i))),64,0,0});
            banks_[0]=bank;banks_[1]=std::move(bank);
            for(auto& m:mixers_) m.resetStatistics();
        }
        if(!SDL_InitSubSystem(SDL_INIT_AUDIO)) {SDL_Log("Original ending audio: %s",SDL_GetError());return;}
        initialized_=true;
        const SDL_AudioSpec spec{SDL_AUDIO_F32,1,5000};
        stream_=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,feed,this);
        if(!stream_ || !SDL_ResumeAudioStreamDevice(stream_)) {
            SDL_Log("Original ending audio device: %s",SDL_GetError());
            if(stream_) SDL_DestroyAudioStream(stream_);
            stream_=nullptr;
        }
    }
    ~EndingAudio() {
        if(stream_) {SDL_SetAudioStreamGetCallback(stream_,nullptr,nullptr);SDL_DestroyAudioStream(stream_);}
        if(initialized_) SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
    void cues(const std::vector<original_ending::Cue>& cues) {
        if(!stream_ || !SDL_LockAudioStream(stream_)) return;
        for(const auto& c:cues) {
            auto& m=mixers_[c.voice];m.clear();
            const auto id=static_cast<unsigned>(c.sound);
            if(id) {banks_[c.voice][id].volume=c.volume;m.play(id);}
        }
        SDL_UnlockAudioStream(stream_);
    }
    void pause(bool paused) {
        if(stream_) {
            if(paused) SDL_PauseAudioStreamDevice(stream_);
            else SDL_ResumeAudioStreamDevice(stream_);
        }
    }
private:
    static void SDLCALL feed(void* user,SDL_AudioStream* stream,int additional,int) {
        auto& self=*static_cast<EndingAudio*>(user);
        std::array<float,512> a{},b{};
        while(additional>0) {
            const int frames=std::min<int>(512,(additional-1)/sizeof(float)+1);
            self.mixers_[0].render(a.data(),frames);self.mixers_[1].render(b.data(),frames);
            for(int i=0;i<frames;++i) a[i]=std::clamp(a[i]+b[i],-1.f,1.f);
            if(!SDL_PutAudioStreamData(stream,a.data(),frames*int(sizeof(float)))) return;
            additional-=frames*int(sizeof(float));
        }
    }
    std::array<audio::Bank,2> banks_;
    std::array<audio::Mixer,2> mixers_;
    SDL_AudioStream* stream_=nullptr;
    bool initialized_=false;
};
struct RenderState {
    SDL_Renderer* renderer;
    int width=0,height=0;
    float x=1,y=1;
    SDL_RendererLogicalPresentation mode=SDL_LOGICAL_PRESENTATION_DISABLED;
    Uint8 r=0,g=0,b=0,a=255;
    explicit RenderState(SDL_Renderer* target):renderer(target) {
        if(!SDL_GetRenderLogicalPresentation(renderer,&width,&height,&mode) ||
           !SDL_GetRenderScale(renderer,&x,&y) || !SDL_GetRenderDrawColor(renderer,&r,&g,&b,&a))
            throw std::runtime_error(SDL_GetError());
    }
    ~RenderState() {
        SDL_SetRenderLogicalPresentation(renderer,width,height,mode);
        SDL_SetRenderScale(renderer,x,y);SDL_SetRenderDrawColor(renderer,r,g,b,a);
    }
};
}
StartupCreditsResult showOriginalEnding(SDL_Window* window,SDL_Renderer* renderer,
    const std::filesystem::path& directory,original_ending::EntryImage entry) {
    try {
        original_ending::Presentation ending(original_ending::loadResources(directory),std::move(entry));
        RenderState restore(renderer);
        if(!SDL_SetRenderLogicalPresentation(renderer,320,200,SDL_LOGICAL_PRESENTATION_LETTERBOX) ||
           !SDL_SetRenderScale(renderer,1,1)) throw std::runtime_error(SDL_GetError());
        std::unique_ptr<SDL_Texture,decltype(&SDL_DestroyTexture)> texture(
            SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_STREAMING,320,200),SDL_DestroyTexture);
        if(!texture || !SDL_SetTextureScaleMode(texture.get(),SDL_SCALEMODE_NEAREST)) throw std::runtime_error(SDL_GetError());
        EndingAudio audio(directory);
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
