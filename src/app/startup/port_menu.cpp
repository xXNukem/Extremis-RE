#include <cstddef>
#include <functional>
#include <filesystem>
#include <optional>
#include <utility>
#include "extremis/render/first_person/presentation/view_scale.h"
#include "app/startup/port_menu.h"
#include "app/startup/port_menu_state.h"
#include "port_menu_view.h"
#include "extremis/resources/audio_data.h"
#include "extremis/ui/original_font.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <vector>

namespace {
using app::menu_detail::hash32;
using app::menu_detail::renderPortMenu;

#include "port_menu_data_pulse.inc"
#include "port_menu_navigation.inc"

// SDL menu owns the logical view and restores its caller's rendering state.
struct RenderRestore {
    SDL_Renderer* r;
    int width=0,height=0;
    float x=1.f,y=1.f;
    SDL_RendererLogicalPresentation mode=SDL_LOGICAL_PRESENTATION_DISABLED;
    SDL_BlendMode blend=SDL_BLENDMODE_NONE;
    Uint8 red=0,green=0,blue=0,alpha=255;
    bool saved=false;
    explicit RenderRestore(SDL_Renderer* target):r(target) {
        saved=SDL_GetRenderLogicalPresentation(r,&width,&height,&mode) &&
              SDL_GetRenderScale(r,&x,&y) && SDL_GetRenderDrawBlendMode(r,&blend) &&
              SDL_GetRenderDrawColor(r,&red,&green,&blue,&alpha);
    }
    ~RenderRestore() {
        if (!saved) return;
        SDL_SetRenderLogicalPresentation(r,width,height,mode);
        SDL_SetRenderScale(r,x,y); SDL_SetRenderDrawBlendMode(r,blend);
        SDL_SetRenderDrawColor(r,red,green,blue,alpha);
    }
};

struct InputReset {
    ~InputReset() {
        SDL_ResetKeyboard();
        SDL_FlushEvents(SDL_EVENT_KEY_DOWN,SDL_EVENT_KEY_UP);
        SDL_FlushEvents(SDL_EVENT_MOUSE_MOTION,SDL_EVENT_MOUSE_WHEEL);
    }
};

// The original DOS code-entry key sound is BANK.DSS effect 28.
// Read it from the player's own DATA directory; never embed copyrighted game audio.
// The previously approved menu hover cue uses its independent 48 kHz stream.
class MenuAudio {
    struct OriginalSound {
        std::vector<float> pcm;
        SDL_AudioStream* stream=nullptr;
    };
public:
    MenuAudio(const std::filesystem::path& dataDirectory, float gain) {
        if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) return;
        initialized_=true;
        const SDL_AudioSpec hoverSpec{SDL_AUDIO_F32,1,48000};
        stream_=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&hoverSpec,nullptr,nullptr);
        if (stream_ && (!SDL_SetAudioStreamGain(stream_,gain) || !SDL_ResumeAudioStreamDevice(stream_))) {
            SDL_DestroyAudioStream(stream_); stream_=nullptr;
        }
        if (dataDirectory.empty()) return;
        try {
            const auto bank=audio::loadBank(dataDirectory/"BANK.DSS");
            // Both effects belong to the original DOS cabinet interface:
            // 28 = code-entry key, 12 = cabinet selection confirmation.
            constexpr std::array<std::size_t,2> effectIds{{28,12}};
            for (std::size_t index=0;index<effectIds.size();++index) {
                const auto id=effectIds[index];
                if (bank.size()<=id || bank[id].sample.pcm.empty()) {
                    SDL_Log("Port menu: BANK.DSS lacks original effect %zu",id);
                    continue;
                }
                const auto& effect=bank[id];
                auto& sound=originalSounds_[index];
                const auto& pcm=effect.sample.pcm;
                sound.pcm.reserve(pcm.size());
                // Same bank volume and fade ramps as the DOS audio player.
                const float sampleGain=float(effect.volume)/256.f;
                for (std::size_t i=0;i<pcm.size();++i) {
                    const float rise=std::min(1.f,float(i+1)/32.f);
                    const float fall=std::min(1.f,float(pcm.size()-i)/32.f);
                    sound.pcm.push_back(pcm[i]*sampleGain*std::min(rise,fall));
                }
                const SDL_AudioSpec spec{SDL_AUDIO_F32,1,int(effect.sample.rate)};
                sound.stream=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,nullptr,nullptr);
                if (sound.stream && (!SDL_SetAudioStreamGain(sound.stream,gain) || !SDL_ResumeAudioStreamDevice(sound.stream))) {
                    SDL_DestroyAudioStream(sound.stream); sound.stream=nullptr;
                }
                if (!sound.stream) SDL_Log("Port menu original BANK[%zu] failed: %s",id,SDL_GetError());
            }
        } catch (const std::exception& e) {
            SDL_Log("Port menu original BANK.DSS key unavailable: %s",e.what());
        }
    }
    ~MenuAudio() {
        for (auto& sound:originalSounds_)
            if (sound.stream) SDL_DestroyAudioStream(sound.stream);
        if (stream_) SDL_DestroyAudioStream(stream_);
        if (initialized_) SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
    void setVolume(float gain) {
        if (stream_) SDL_SetAudioStreamGain(stream_,gain);
        for (auto& sound : originalSounds_)
            if (sound.stream) SDL_SetAudioStreamGain(sound.stream,gain);
    }
    void hover() {
        if (SDL_GetTicksNS() < transitionProtectedUntil_) return;
        play(dataPulse);
    }
    // DOS interfaceKey() uses BANK[28] for both code-entry and delete keys.
    void enter() { transitionProtectedUntil_=SDL_GetTicksNS()+180000000ULL; playOriginal(0); }
    void back() { transitionProtectedUntil_=SDL_GetTicksNS()+180000000ULL; playOriginal(0); }
    // Cabinet confirmation BANK[12]: a value actually changed, not just a row selection.
    void adjust() {
        transitionProtectedUntil_=SDL_GetTicksNS()+180000000ULL;
        if (stream_) SDL_ClearAudioStream(stream_); // Do not mask the adjustment with hover.
        playOriginal(1);
    }

private:
    void playOriginal(std::size_t index) {
        auto& sound=originalSounds_[index];
        if (!sound.stream || sound.pcm.empty()) return;
        if (!SDL_ClearAudioStream(sound.stream) ||
            !SDL_PutAudioStreamData(sound.stream,sound.pcm.data(),int(sound.pcm.size()*sizeof(float))))
            SDL_Log("Port menu original BANK sound failed: %s",SDL_GetError());
    }
    template<std::size_t N>
    void play(const std::array<std::int16_t,N>& source) {
        if (!stream_) return;
        std::array<float,N> pcm{};
        for (std::size_t i=0;i<N;++i) pcm[i]=float(source[i])/32768.f;
        if (!SDL_ClearAudioStream(stream_) ||
            !SDL_PutAudioStreamData(stream_,pcm.data(),int(pcm.size()*sizeof(float))))
            SDL_Log("Port menu hover audio failed: %s",SDL_GetError());
    }
    SDL_AudioStream* stream_=nullptr;
    std::array<OriginalSound,2> originalSounds_{};
    Uint64 transitionProtectedUntil_=0;
    bool initialized_=false;
};

} // namespace

// Returns physical pixels; SDL3 desktop modes use logical size plus
// pixel_density on Retina / high-DPI screens.
static std::pair<int,int> activeDisplaySize(SDL_Window* window)
{
    const SDL_DisplayID display=SDL_GetDisplayForWindow(window);
    const SDL_DisplayMode* mode=display?SDL_GetDesktopDisplayMode(display):nullptr;
    if(mode && mode->w>0 && mode->h>0) {
        const float density=mode->pixel_density>0.f?mode->pixel_density:1.f;
        return {int(std::lround(mode->w*density)),int(std::lround(mode->h*density))};
    }
    int w=0,h=0;
    if(SDL_GetWindowSizeInPixels(window,&w,&h) && w>0 && h>0)
        return {w,h};
    return {1920,1080};
}

// Read real display modes every time a menu is opened, including Retina density.
// Do not cache SDL_DisplayMode pointers, which belong to temporary SDL arrays.
// Pixel dimensions are taken from the display currently containing the
// window, not the primary monitor. SDL Retina density is applied exactly once.
static std::vector<PortVideoMode> availableVideoModes(SDL_Window* window,unsigned aspectChoice)
{
    const SDL_DisplayID display=SDL_GetDisplayForWindow(window);
    auto [maxW,maxH]=activeDisplaySize(window);
    if(display) {
        int count=0;
        SDL_DisplayMode** modes=SDL_GetFullscreenDisplayModes(display,&count);
        if(modes) {
            for(int i=0;i<count;++i) {
                const auto* m=modes[i];
                if(!m || m->w<=0 || m->h<=0) continue;
                const double density=m->pixel_density>0.f ? m->pixel_density:1.f;
                const int w=int(std::lround(m->w*density));
                const int h=int(std::lround(m->h*density));
                if(w>0 && h>0 &&
                   std::uint64_t(w)*h>std::uint64_t(maxW)*maxH) {
                    maxW=w; maxH=h;
                }
            }
            SDL_free(modes);
        }
    }
    const double aspect = extremis::selectedAspect(aspectChoice, maxW, maxH);
    // A DOS framebuffer of 320x200 corresponds to 4:3 after CRT pixel
    // correction. n x DOS uses 240*n physical lines at 4:3 on screen.
    const double fittingHeight=std::min(double(maxH),double(maxW)/aspect);
    const int integerMaximum=std::clamp(int(fittingHeight/240.0),1,maxDosResolutionScale);
    std::vector<PortVideoMode> result;
    for(int scale=2;scale<=integerMaximum;++scale)
        result.push_back({320*scale,200*scale,1.f,false,true});
    // Final entry is EXACTLY the maximum current display mode, including
    // fractional DOS scaling where needed (e.g. 1920x1080 at 16:9).
    result.push_back({maxW,maxH,1.f,true});
    return result;
}

// The monitor stays in desktop fullscreen. The selected value controls
// CPU-rendered 3D detail, not the physical scanout mode.
// Internal resolution is intentionally NOT a hardware display-mode switch.
// Even 320x200 runs in desktop fullscreen; only CPU scene detail changes.
static bool applyVideoWindowSize(SDL_Window* window,PortMenuState& state,
                          VideoRenderMode& output)
{
    if(SDL_GetWindowFullscreenMode(window)) {
        if(!SDL_SetWindowFullscreenMode(window,nullptr) ||
           !SDL_SetWindowFullscreen(window,true) || !SDL_SyncWindow(window)) {
            SDL_Log("VIDEO: cannot return to desktop fullscreen: %s",SDL_GetError());
            return false;
        }
    }
    int outputWidth=0,outputHeight=0;
    if (SDL_GetWindowSizeInPixels(window,&outputWidth,&outputHeight))
        state.setAppliedVideoSize(outputWidth,outputHeight);
    output=state.selectedRenderMode();
    const auto [displayW,displayH]=activeDisplaySize(window);
    SDL_Log("VORTEX VIDEO: selected %d x %d %s; renderer scale=%d; display max=%d x %d",
            output.requestedSize.width,output.requestedSize.height,
            output.nativeMax?"(MAX)":"(DOS pixels)",output.renderScale,displayW,displayH);
    return true;
}

static void configureVideoModes(SDL_Window* window, PortMenuState& state)
{
    const auto [width,height]=activeDisplaySize(window);
    state.setDisplaySize(width,height);
    int outputWidth=0,outputHeight=0;
    if (SDL_GetWindowSizeInPixels(window,&outputWidth,&outputHeight))
        state.setAppliedVideoSize(outputWidth,outputHeight);
    state.setAvailableVideoModes(availableVideoModes(window,state.aspectChoice()));
    state.restoreResolution();
}

void initializePortVideo(SDL_Window* window, SDL_Renderer* renderer, UserSettings& settings)
{
    PortMenuState state(false,settings,renderer);
    configureVideoModes(window,state);
    settings.render.mode=state.selectedRenderMode();
    applyPreferredVSync(renderer,settings);
}

PortMenuResult showPortMenu(SDL_Window* window,SDL_Renderer* renderer,
                            const std::filesystem::path& dataDirectory,bool inGame,
                            UserSettings& settings, const std::filesystem::path& settingsPath,
                            const std::function<void(float)>& onVolumeChanged)
{
    if (!window || !renderer) return PortMenuChoice::Error;
    RenderRestore restore(renderer);
    if (!restore.saved ||
        !SDL_SetRenderLogicalPresentation(renderer,640,400,SDL_LOGICAL_PRESENTATION_LETTERBOX) ||
        !SDL_SetRenderScale(renderer,1.f,1.f) ||
        !SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND))
        return PortMenuChoice::Error;

    InputReset resetInput;
    std::unique_ptr<original_font::Font> font;
    if (!dataDirectory.empty()) {
        try {font=std::make_unique<original_font::Font>(renderer,dataDirectory/"CASQUE.SPR");}
        catch (const std::exception& e) {SDL_Log("Port menu font fallback: %s",e.what());}
    }
    MenuAudio audio(dataDirectory,settings.audioGain());
    const auto windowId=SDL_GetWindowID(window);
    PortMenuState state(inGame,settings,renderer);
    configureVideoModes(window,state);
    auto& mode=settings.render.mode;
    const auto storedMode=mode;
    mode=state.selectedRenderMode();
    if (storedMode.renderScale!=mode.renderScale || storedMode.nativeMax!=mode.nativeMax)
        saveUserSettings(settingsPath,settings);
    unsigned selectedResolution=state.resolutionChoice();
    unsigned selectedAspect=state.aspectChoice();
    double seconds=0.,selectionChangedAt=0.;
    const std::uint32_t effectSeed=hash32(std::uint32_t(SDL_GetTicksNS()));
    std::uint32_t selectionSeed=hash32(effectSeed ^ 0x51409a6du);
    unsigned selectionCount=0;
    const auto valuesOnPage=[&]() {
        std::string labels;
        for (unsigned row=0;row<state.rowCount();++row) {
            labels+=state.label(row);
            labels+='\n';
        }
        return labels;
    };
    const auto selectionChanged=[&](PortMenuPage previousPage,unsigned previousSelection,
                                    const std::string& previousValues) {
        const bool changedPage=state.page()!=previousPage;
        if (state.page()==PortMenuPage::Video) {
            const bool aspectChanged=state.aspectChoice()!=selectedAspect;
            if (aspectChanged) configureVideoModes(window,state);
            if (state.resolutionChoice()!=selectedResolution || aspectChanged) {
                const bool supported=applyVideoWindowSize(window,state,mode);
                state.setVideoModeAvailable(supported);
                if (supported) {
                    selectedAspect=state.aspectChoice();
                    selectedResolution=state.resolutionChoice();
                } else {
                    state.setVideoChoices(selectedAspect,selectedResolution);
                    configureVideoModes(window,state);
                }
            }
        }
        const bool changedValue=!changedPage && valuesOnPage()!=previousValues;
        if (changedValue && state.page()!=PortMenuPage::Password) {
            if (state.page()==PortMenuPage::Audio) {
                audio.setVolume(settings.audioGain());
                if (onVolumeChanged) onVolumeChanged(settings.audioGain());
            }
            saveUserSettings(settingsPath,settings);
        }
        if (!changedPage && !changedValue && state.selection()==previousSelection) return;
        selectionChangedAt=seconds;
        selectionSeed=hash32(effectSeed ^ ++selectionCount*0x9e3779b9u ^
                             std::uint32_t(SDL_GetTicksNS()));
        if (changedPage) {
            // Page depth, rather than row movement, determines the transition cue.
            const auto depth=[](PortMenuPage page) {
                return page==PortMenuPage::Main?0:page==PortMenuPage::Options?1:2;
            };
            if (depth(state.page())>depth(previousPage)) audio.enter();
            else audio.back();
        } else if (changedValue) audio.adjust();
        else audio.hover();
    };
    struct PasswordInput {
        SDL_Window* window;
        bool active = false;
        void set(bool enabled) {
            if (enabled==active) return;
            active=enabled;
            if (enabled) SDL_StartTextInput(window);
            else SDL_StopTextInput(window);
        }
        ~PasswordInput() { set(false); }
    } passwordInput{window};
    std::optional<Uint64> responseStarted;
    bool focused=(SDL_GetWindowFlags(window)&SDL_WINDOW_INPUT_FOCUS)!=0;
    auto previous=SDL_GetTicksNS();
    while (true) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type==SDL_EVENT_QUIT ||
                (event.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID==windowId))
                return PortMenuChoice::Exit;
            if (event.type==SDL_EVENT_WINDOW_FOCUS_LOST && event.window.windowID==windowId)
                focused=false;
            if (event.type==SDL_EVENT_WINDOW_FOCUS_GAINED && event.window.windowID==windowId)
                focused=true;
            if (event.type==SDL_EVENT_RENDER_DEVICE_RESET && event.render.windowID==windowId)
                applyPreferredVSync(renderer,settings);
            if (!focused) continue;
            if (event.type==SDL_EVENT_KEY_DOWN && event.key.windowID==windowId) {
                const auto old=state.selection();
                const auto oldPage=state.page();
                const auto oldValues=valuesOnPage();
                const auto choice=state.key(event.key.scancode,event.key.repeat);
                if (choice!=PortMenuChoice::None) return choice;
                selectionChanged(oldPage,old,oldValues);
                passwordInput.set(state.page()==PortMenuPage::Password && !state.passwordSubmitted());
            } else if (event.type==SDL_EVENT_TEXT_INPUT && event.text.windowID==windowId) {
                state.typePassword(event.text.text);
            } else if (event.type==SDL_EVENT_MOUSE_MOTION && event.motion.windowID==windowId) {
                if (SDL_ConvertEventToRenderCoordinates(renderer,&event)) {
                    const auto oldPage=state.page();
                    const auto oldSelection=state.selection();
                    const auto oldValues=valuesOnPage();
                    if (state.pointerMove(event.motion.x,event.motion.y))
                        selectionChanged(oldPage,oldSelection,oldValues);
                }
            } else if (event.type==SDL_EVENT_MOUSE_BUTTON_DOWN &&
                       event.button.windowID==windowId && event.button.button==SDL_BUTTON_LEFT) {
                if (SDL_ConvertEventToRenderCoordinates(renderer,&event)) {
                    const auto old=state.selection();
                    const auto oldPage=state.page();
                    const auto oldValues=valuesOnPage();
                    const auto choice=state.pointerClick(event.button.x,event.button.y);
                    selectionChanged(oldPage,old,oldValues);
                    if (choice!=PortMenuChoice::None) return choice;
                    passwordInput.set(state.page()==PortMenuPage::Password && !state.passwordSubmitted());
                }
            }
        }
        const auto now=SDL_GetTicksNS();
        if (state.passwordSubmitted()) {
            if (!responseStarted) responseStarted=now;
            // Approximate the DOS hold of 140 VGA retraces at 70 Hz.
            if (now-*responseStarted>=2000000000ull) {
                if (const auto* checkpoint=state.finishPasswordResponse())
                    return {PortMenuChoice::Continue,checkpoint};
                responseStarted.reset();
            }
        }
        if (focused) seconds+=std::min(double(now-previous)*1e-9,.06);
        previous=now;
        if (!renderPortMenu(renderer,font.get(),state,seconds,seconds-selectionChangedAt,
                    effectSeed,selectionSeed)) {
            SDL_Log("Port menu rendering failed: %s",SDL_GetError());
            return PortMenuChoice::Error;
        }
        SDL_Delay(16);
    }
}
