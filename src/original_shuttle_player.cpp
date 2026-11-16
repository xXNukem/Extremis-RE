#include "original_shuttle_player.h"
#include "original_shuttle.h"
#include "audio_mixer.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>
#include <utility>

namespace {
// One explicitly owned cinematic voice, using the existing DSS reader and mixer.
class ShuttleAudio {
public:
    explicit ShuttleAudio(const std::filesystem::path& directory)
        : mixer_(bank_) {
        {
            audio::Bank bank(1); // index zero is the stop sentinel, never played
            for(unsigned i=0;i<10;++i)
                bank.push_back({audio::loadSample(directory/original_shuttle::stage(static_cast<original_shuttle::Scene>(i)).sound),48,0,0});
            bank_=std::move(bank);
            mixer_.resetStatistics();
        }
        if(!SDL_InitSubSystem(SDL_INIT_AUDIO)) {SDL_Log("Original shuttle audio: %s",SDL_GetError());return;}
        initialized_=true;
        const SDL_AudioSpec spec{SDL_AUDIO_F32,1,5000};
        stream_=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,feed,this);
        if(!stream_ || !SDL_ResumeAudioStreamDevice(stream_)) {
            SDL_Log("Original shuttle audio device: %s",SDL_GetError());
            if(stream_) SDL_DestroyAudioStream(stream_);
            stream_=nullptr;
        }
    }
    ~ShuttleAudio() {
        if(stream_) {SDL_SetAudioStreamGetCallback(stream_,nullptr,nullptr);SDL_DestroyAudioStream(stream_);}
        if(initialized_) SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
    void cues(const std::vector<int>& cues) {
        if(!stream_ || !SDL_LockAudioStream(stream_)) return;
        for(const auto& c:cues) {
            auto& m=mixer_;m.clear();
            if(c>=0) m.play(unsigned(c)+1);
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
        auto& self=*static_cast<ShuttleAudio*>(user);
        std::array<float,512> a{};
        while(additional>0) {
            const int frames=std::min<int>(512,(additional-1)/sizeof(float)+1);
            self.mixer_.render(a.data(),frames);
            if(!SDL_PutAudioStreamData(stream,a.data(),frames*int(sizeof(float)))) return;
            additional-=frames*int(sizeof(float));
        }
    }
    audio::Bank bank_;
    audio::Mixer mixer_;
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
StartupCreditsResult showOriginalShuttle(SDL_Window* window,SDL_Renderer* renderer,
    const std::filesystem::path& directory,bool inheritedAnmEscape) {
    try {
        original_shuttle::Presentation ending(original_shuttle::loadResources(directory),inheritedAnmEscape);
        RenderState restore(renderer);
        if(!SDL_SetRenderLogicalPresentation(renderer,320,200,SDL_LOGICAL_PRESENTATION_LETTERBOX) ||
           !SDL_SetRenderScale(renderer,1,1)) throw std::runtime_error(SDL_GetError());
        std::unique_ptr<SDL_Texture,decltype(&SDL_DestroyTexture)> texture(
            SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_STREAMING,320,200),SDL_DestroyTexture);
        if(!texture || !SDL_SetTextureScaleMode(texture.get(),SDL_SCALEMODE_NEAREST)) throw std::runtime_error(SDL_GetError());
        ShuttleAudio audio(directory);
        SDL_Log("Original shuttle: nominal PIT, 70 Hz retrace and adapted 70 Hz CPU composition (see elevator-cinematics.md)");
        bool focused=(SDL_GetWindowFlags(window)&SDL_WINDOW_INPUT_FOCUS)!=0;
        audio.pause(!focused);
        const auto id=SDL_GetWindowID(window);
        auto previous=SDL_GetTicksNS();
        while(ending.scene()!=original_shuttle::Scene::Done) {
            SDL_Event event;bool escape=false;
            while(SDL_PollEvent(&event)) {
                if(event.type==SDL_EVENT_QUIT || (event.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID==id))
                    return StartupCreditsResult::Quit;
                if(event.type==SDL_EVENT_KEY_DOWN && event.key.key==SDLK_ESCAPE) escape=true;
                if(event.type==SDL_EVENT_WINDOW_FOCUS_LOST && event.window.windowID==id) {focused=false;audio.pause(true);}
                if(event.type==SDL_EVENT_WINDOW_FOCUS_GAINED && event.window.windowID==id) {focused=true;audio.pause(false);}
            }
            const auto now=SDL_GetTicksNS();
            if(focused) ending.advance(double(now-previous)/1e9,escape);
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
        SDL_Log("Original shuttle failed: %s",e.what());
        return StartupCreditsResult::Error;
    }
}
