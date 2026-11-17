#include "port_menu.h"
#include "audio_data.h"
#include "original_font.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace {
constexpr float menuLeft=142.f, menuRight=498.f;
constexpr float submenuLeft=84.f, submenuRight=556.f;
constexpr float firstRow=170.f, rowSpacing=41.f;
constexpr float submenuFirstRow=116.f, submenuRowSpacing=46.f, rowHeight=34.f;
constexpr double pi=3.14159265358979323846;
constexpr SDL_Color phosphor{183,255,90,255};
constexpr SDL_Color pale{214,255,171,255};
constexpr SDL_Color dim{93,125,77,255};
constexpr SDL_Color frame{62,112,52,255};
constexpr SDL_Color dark{4,10,6,255};
constexpr std::array<std::string_view,4> mainLabels{{"NEW GAME","LOAD GAME","OPTIONS","EXIT"}};
constexpr std::array<std::string_view,5> optionsLabels{{"VIDEO","AUDIO","CONTROLS","LANGUAGE","BACK"}};
constexpr std::array<std::string_view,4> aspectValues{{"4:3","16:9","21:9","AUTO"}};
constexpr std::array<std::string_view,5> resolutions{{"640 X 400","1024 X 768","1280 X 720","1920 X 1080","2560 X 1440"}};
constexpr std::array<std::string_view,3> languageLabels{{"FRENCH","ENGLISH","SPANISH"}};

float rowTop(unsigned row,unsigned count,bool submenu)
{
    if (submenu)
        return submenuFirstRow+(5.f-float(count))*submenuRowSpacing*.5f+
               float(row)*submenuRowSpacing;
    return firstRow+(4.f-float(count))*rowSpacing*.5f+float(row)*rowSpacing;
}

#include "port_menu_data_pulse.inc"
#include "port_menu_navigation.inc"

std::uint32_t hash32(std::uint32_t x)
{
    x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;return x^(x>>16);
}

float pulse(double seconds, float speed, float strength)
{
    return 1.f-strength*.5f+strength*.5f*std::sin(float(seconds)*speed);
}

bool ink(SDL_Renderer* r, SDL_Color c, Uint8 opacity=255)
{
    return SDL_SetRenderDrawColor(r,c.r,c.g,c.b,opacity);
}

bool line(SDL_Renderer* r, float x1,float y1,float x2,float y2,
          SDL_Color c,Uint8 alpha=255)
{
    return ink(r,c,alpha) && SDL_RenderLine(r,x1,y1,x2,y2);
}

bool fill(SDL_Renderer* r, SDL_FRect rect, SDL_Color c, Uint8 alpha)
{
    return ink(r,c,alpha) && SDL_RenderFillRect(r,&rect);
}

bool textAt(SDL_Renderer* r, const original_font::Font* font, std::string_view text,
            float x,float y,float scale, SDL_Color tint,float opacity=1.f)
{
    if (font) return font->draw(r,text,x,y,scale,tint,opacity);
    // Asset-free fallback; source text is null-terminated before passing to SDL.
    const std::string copy(text);
    return ink(r,tint,Uint8(std::clamp(opacity,0.f,1.f)*255.f)) &&
           SDL_RenderDebugText(r,x,y,copy.c_str());
}

float textWidth(const original_font::Font* font,std::string_view s,float scale)
{
    return (font?font->measure(s):float(s.size())*8.f)*scale;
}

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
    explicit MenuAudio(const std::filesystem::path& dataDirectory) {
        if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) return;
        initialized_=true;
        const SDL_AudioSpec hoverSpec{SDL_AUDIO_F32,1,48000};
        stream_=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&hoverSpec,nullptr,nullptr);
        if (stream_ && !SDL_ResumeAudioStreamDevice(stream_)) {
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
                const float gain=float(effect.volume)/256.f;
                for (std::size_t i=0;i<pcm.size();++i) {
                    const float rise=std::min(1.f,float(i+1)/32.f);
                    const float fall=std::min(1.f,float(pcm.size()-i)/32.f);
                    sound.pcm.push_back(pcm[i]*gain*std::min(rise,fall));
                }
                const SDL_AudioSpec spec{SDL_AUDIO_F32,1,int(effect.sample.rate)};
                sound.stream=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,nullptr,nullptr);
                if (sound.stream && !SDL_ResumeAudioStreamDevice(sound.stream)) {
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

bool drawCRT(SDL_Renderer* r,double seconds,std::uint32_t seed)
{
    if (!ink(r,dark) || !SDL_RenderClear(r)) return false;

    // Entire screen is a CRT surface: no frame, grid, or moving full-width band.
    for (int y=0;y<400;y+=4) {
        const float center=1.f-std::abs(float(y)-196.f)/235.f;
        const float drift=.5f+.5f*std::sin(float(seconds)*.65f+float(y)*.019f);
        const Uint8 alpha=Uint8(16.f+15.f*std::max(0.f,center)+7.f*drift);
        if (!fill(r,{0.f,float(y),640.f,4.f},{21,50,15,255},alpha)) return false;
    }
    for (int y=0;y<400;y+=3)
        if (!line(r,0.f,float(y),640.f,float(y),{0,0,0,255},32)) return false;

    // Soft edge falloff; it must not read as a decorative rectangular bezel.
    for (int x=0;x<640;x+=8) {
        const float edge=std::abs(float(x+4)-320.f)/320.f;
        if (!fill(r,{float(x),0.f,8.f,400.f},{0,0,0,255},
                  Uint8(36.f*edge*edge))) return false;
    }
    for (int y=0;y<400;y+=8) {
        const float edge=std::abs(float(y+4)-200.f)/200.f;
        if (!fill(r,{0.f,float(y),640.f,8.f},{0,0,0,255},
                  Uint8(29.f*edge*edge))) return false;
    }

    // Independent irregular video-memory faults. Each slot has its own random
    // position, duration, count, and width; no modulo-scheduled repeated sweep.
    const auto tick=std::uint32_t(std::floor(seconds*15.));
    const auto event=hash32(seed ^ (tick*0x9e3779b9u));
    if ((event&255u)<76u) {
        const unsigned count=2u+((event>>12)&7u);
        for (unsigned i=0;i<count;++i) {
            const auto h=hash32(event ^ (i+1u)*0x85ebca6bu);
            const float y=float((h>>13)%398u)+.5f;
            const float x=float((h>>4)%608u);
            const float len=10.f+float((h>>20)%155u);
            const auto alpha=Uint8(13u+(h&47u));
            if (!line(r,x,y,std::min(639.f,x+len),y,phosphor,alpha)) return false;
            if ((h&7u)==1u &&
                !line(r,x+3.f,y+1.f,std::min(639.f,x+len*.75f),y+1.f,
                      {0,0,0,255},65)) return false;
        }
    }
    return true;
}

bool drawTitle(SDL_Renderer* r,const original_font::Font* font,double time,
               std::uint32_t seed)
{
    constexpr std::string_view title="IN EXTREMIS";
    const float scale=3.25f;
    const float span=textWidth(font,title,scale);
    const float x=320.f-span*.5f;
    const float y=74.f;
    const auto tick=std::uint32_t(std::floor(time*20.));
    const auto fault=hash32(seed ^ tick*0x9e3779b9u);
    const bool rupture=(fault&15u)<6u; // Short, frequent irregular corruption.
    const float jitter=rupture?float(int((fault>>12)%17u)-8)*.72f:0.f;
    const float brightness=pulse(time,2.15f,.095f)*(rupture && (fault&255u)<22u?.48f:1.f);

    // Stable phosphor logo, with a displaced image only during signal failure.
    if (!textAt(r,font,title,x-2.f+jitter*.4f,y+1.f,scale,{48,111,47,255},.43f) ||
        !textAt(r,font,title,x+2.f-jitter*.4f,y-1.f,scale,{116,216,83,255},.23f) ||
        !textAt(r,font,title,x+jitter*.16f,y,scale,phosphor,brightness)) return false;

    if (!rupture) return true;
    // True horizontal glyph corruption: black out short scan bands, then
    // reprint their clipped strokes at unpredictable horizontal offsets.
    const unsigned bands=2u+((fault>>8)&3u);
    for (unsigned i=0;i<bands;++i) {
        const auto h=hash32(fault ^ (i+1u)*0xc2b2ae35u);
        const int bandY=int(y)+int((h>>14)%32u);
        const int bandHeight=2+int((h>>24)%6u);
        const SDL_Rect clip{int(x)-18,bandY,int(span)+36,bandHeight};
        if (!SDL_SetRenderClipRect(r,&clip)) return false;
        const float displacement=float(int((h>>4)%27u)-13);
        const bool bandOK=
            fill(r,{float(clip.x),float(clip.y),float(clip.w),float(clip.h)},dark,235) &&
            textAt(r,font,title,x+displacement,y,scale,
                   (h&1u)?phosphor:pale,.70f);
        const bool restoreOK=SDL_SetRenderClipRect(r,nullptr);
        if (!bandOK || !restoreOK) return false;
    }
    // Stray memory shears stay local to the title rather than forming long
    // UI-decoration bars beneath it.
    for (unsigned i=0;i<5;++i) {
        const auto h=hash32(fault ^ (i+7u)*0x27d4eb2du);
        const float sx=x+float(h%std::uint32_t(std::max(1.f,span)));
        const float sy=y+float((h>>13)%36u);
        const float length=5.f+float((h>>21)%48u);
        if (!line(r,sx,sy,std::min(x+span,sx+length),sy,phosphor,
                  Uint8(40u+(h&63u)))) return false;
    }
    return true;
}

// Only submenus have this header. Its restrained ghosting and momentary
// horizontal shears belong to the same damaged phosphor display as the logo.
bool drawPageHeader(SDL_Renderer* r,const original_font::Font* font,
                    const PortMenuState& state,double time,std::uint32_t seed)
{
    if (state.page()==PortMenuPage::Main) return true;
    const std::string_view heading=state.pageTitle();
    constexpr float x=52.f,y=37.f,scale=2.45f;
    const float width=textWidth(font,heading,scale);
    const auto tick=std::uint32_t(std::floor(time*18.));
    const auto noise=hash32(seed ^ (tick+5u)*0x85ebca6bu);
    const bool fault=(noise&15u)<4u;
    const float offset=fault?float(int((noise>>10)&15u)-7)*.42f:0.f;
    const float brightness=pulse(time,1.8f,.08f);

    // Short side phosphor mark, not a decorative box or horizontal rule.
    if (!fill(r,{31.f,y+1.f,4.f,23.f},phosphor,Uint8(115.f*brightness)) ||
        !fill(r,{38.f,y+5.f,2.f,13.f},frame,90) ||
        !textAt(r,font,heading,x+1.5f,y+1.5f,scale,{36,91,39,255},.58f) ||
        !textAt(r,font,heading,x+offset,y,scale,phosphor,brightness)) return false;
    if (!fault) return true;

    // Two narrow displaced slices briefly corrupt the actual heading glyphs.
    for (unsigned i=0;i<2;++i) {
        const auto h=hash32(noise ^ (i+1u)*0x9e3779b9u);
        const int row=int(y)+3+int((h>>9)%14u);
        const SDL_Rect clip{int(x)-3,row,int(width)+12,3};
        if (!SDL_SetRenderClipRect(r,&clip)) return false;
        const float dx=float(int((h>>20)&15u)-7)*.8f;
        const bool drawn=textAt(r,font,heading,x+dx,y,scale,pale,.75f);
        const bool restored=SDL_SetRenderClipRect(r,nullptr);
        if (!drawn || !restored) return false;
    }
    return true;
}

// Each focus change seeds a different complete waveform. The trace grows
// left-to-right through the full selection rectangle, then fades away.
// A fresh, asymmetric spike pattern on every focus change. One fast pass
// across the whole row; its sharp peaks decay before the glow settles.
bool drawSelectionECG(SDL_Renderer* r,float rowY,double age,std::uint32_t seed,
                      float leftEdge,float rightEdge)
{
    constexpr double sweep=.56, end=.75;
    if (age<0. || age>=end) return true;
    constexpr unsigned samples=361;
    const float progress=std::clamp(float(age/sweep),0.f,1.f);
    const int count=std::clamp(int(progress*float(samples-1))+1,2,int(samples));
    const float fadeOut=1.f-std::clamp(float((age-.54)/.21),0.f,1.f);
    const float left=leftEdge+6.f,span=rightEdge-leftEdge-12.f;
    const float baseline=rowY+rowHeight*.51f;
    const unsigned spikes=3u+((seed>>11)&3u);
    std::array<SDL_FPoint,samples> points{};
    for (int i=0;i<count;++i) {
        const float t=float(i)/float(samples-1);
        const float x=left+t*span;
        float wave=.20f*std::sin(t*58.f+float(seed&255u)*.047f);
        for (unsigned j=0;j<spikes;++j) {
            const auto h=hash32(seed ^ (j+1u)*0x9e3779b9u);
            const float jitter=(float(h&255u)/255.f-.5f)*.048f;
            const float center=.10f+.80f*(float(j)+.5f)/float(spikes)+jitter;
            const float halfWidth=.009f+float((h>>8)&255u)/26000.f;
            const float distance=std::abs((t-center)/halfWidth);
            // Narrow triangular peaks, never the smooth, rounded blobs of v4.
            const float apex=std::max(0.f,1.f-distance);
            const float amplitude=8.4f+float((h>>16)&255u)/90.f;
            const float polarity=(h&0x1000000u)?-1.f:1.f;
            wave+=polarity*amplitude*apex;
            const float undershoot=std::max(0.f,1.f-std::abs((t-center-halfWidth*1.45f)/halfWidth));
            wave-=polarity*amplitude*.29f*undershoot;
        }
        points[std::size_t(i)]={x,baseline-std::clamp(wave,-12.f,12.f)};
    }
    if (!ink(r,phosphor,Uint8(60.f*fadeOut)) ||
        !SDL_RenderLines(r,points.data(),count) ||
        !ink(r,pale,Uint8(160.f*fadeOut)) ||
        !SDL_RenderLines(r,points.data(),count)) return false;
    const auto head=points[std::size_t(count-1)];
    const SDL_FRect spark{head.x-1.f,head.y-1.f,2.f,2.f};
    return fill(r,spark,pale,Uint8(120.f*fadeOut));
}

bool drawOptions(SDL_Renderer* r,const original_font::Font* font,
                 const PortMenuState& state,double time,double selectionAge,
                 std::uint32_t seed)
{
    const bool submenu=state.page()!=PortMenuPage::Main;
    const float left=submenu?submenuLeft:menuLeft;
    const float right=submenu?submenuRight:menuRight;
    const float width=right-left;
    const auto count=state.rowCount();
    // A single fixed scale for all submenu values, including their longest
    // alternatives. Changing resolution, language or scheme never resizes text.
    constexpr std::array<std::string_view,5> largestLabels{{
        "RESOLUTION : 2560 X 1440", "FLOOR AND CEILING : OFF",
        "CONTROL SCHEME : CLASSIC", "SPANISH - SELECTED",
        "MASTER VOLUME : 100"
    }};
    float reference=1.f;
    for (const auto label:largestLabels) reference=std::max(reference,textWidth(font,label,1.f));
    const float glyphScale=submenu?std::min(1.90f,(width-28.f)/reference):2.05f;
    for (unsigned i=0;i<count;++i) {
        const float y=rowTop(i,count,submenu);
        const bool selected=state.selection()==i;
        const auto label=state.label(i);
        const auto tint=selected?phosphor:pale; // No dimmed placeholder entries.
        if (selected) {
            const float phase=float(time*3.1+double(i)*1.8);
            const float opacity=.64f+.36f*std::sin(phase);
            // Same transparent green field, noise and ECG as the approved v5.
            if (!fill(r,{left,y,width,rowHeight},frame,
                      Uint8(43+21*opacity)) ||
                !fill(r,{left+10.f,y+8.f,3.f,18.f},phosphor,
                      Uint8(140+60*opacity))) return false;
            const float scan=left+24.f+float(std::fmod(time*86.,width-50.f));
            if (!line(r,scan,y+2.f,scan,y+rowHeight-2.f,phosphor,42)) return false;
            const auto tick=std::uint32_t(std::floor(time*24.));
            const auto fault=hash32(seed ^ (tick+1u)*0x85ebca6bu);
            if ((fault&7u)<3u) {
                for (unsigned j=0;j<3;++j) {
                    const auto h=hash32(fault ^ (j+1u)*0xc2b2ae35u);
                    const float x=left+8.f+float((h>>5)%unsigned(width-66.f));
                    const float len=7.f+float((h>>15)%37u);
                    const float tearY=y+4.f+float((h>>21)%26u);
                    if (!line(r,x,tearY,std::min(right-4.f,x+len),tearY,dark,165) ||
                        !line(r,x+float(int((h>>24)%9u)-4),tearY+1.f,
                              std::min(right-4.f,x+len),tearY+1.f,
                              phosphor,120)) return false;
                }
            }
            if (!drawSelectionECG(r,y,selectionAge,seed,left,right)) return false;
        }
        // Scale is fixed for the page; only horizontal centering follows the value.
        const float x=320.f-textWidth(font,label,glyphScale)*.5f;
        const float textY=y+(rowHeight-10.f*glyphScale)*.5f;
        if (selected && !textAt(r,font,label,x+2.f,textY+1.f,glyphScale,phosphor,.20f))
            return false;
        if (!textAt(r,font,label,x,textY,glyphScale,tint,selected?1.f:.80f)) return false;
    }
    return true;
}

bool render(SDL_Renderer* r,const original_font::Font* font,
            const PortMenuState& state,double seconds,double selectionAge,
            std::uint32_t effectSeed,std::uint32_t selectionSeed)
{
    if (!drawCRT(r,seconds,effectSeed) ||
        !(state.page()==PortMenuPage::Main
            ? drawTitle(r,font,seconds,effectSeed)
            : drawPageHeader(r,font,state,seconds,effectSeed)) ||
        !drawOptions(r,font,state,seconds,selectionAge,selectionSeed)) return false;

    // Only the unobtrusive source-port credit remains in the bottom-right corner.
    constexpr std::string_view credit="EXTREMIS RE SOURCE PORT";
    const float scale=.68f;
    const float x=626.f-textWidth(font,credit,scale);
    if (!textAt(r,font,credit,x,382.f,scale,dim,.65f)) return false;
    return SDL_RenderPresent(r);
}
} // namespace

unsigned PortMenuState::rowCount() const
{
    switch(page_) {
    case PortMenuPage::Main: return 4;
    case PortMenuPage::Options: case PortMenuPage::Video: return 5;
    case PortMenuPage::Audio: case PortMenuPage::Controls: return 2;
    case PortMenuPage::Language: return 4;
    }
    return 4;
}

const char* PortMenuState::pageTitle() const
{
    switch(page_) {
    case PortMenuPage::Options: return "OPTIONS";
    case PortMenuPage::Video: return "VIDEO";
    case PortMenuPage::Audio: return "AUDIO";
    case PortMenuPage::Controls: return "CONTROLS";
    case PortMenuPage::Language: return "LANGUAGE";
    case PortMenuPage::Main: break;
    }
    return "";
}

std::string PortMenuState::label(unsigned row) const
{
    if (row>=rowCount()) return "";
    switch(page_) {
    case PortMenuPage::Main: return std::string(mainLabels[row]);
    case PortMenuPage::Options: return std::string(optionsLabels[row]);
    case PortMenuPage::Video:
        switch(row) {
        case 0: return "ASPECT RATIO : "+std::string(aspectValues[aspect_]);
        case 1: return "RESOLUTION : "+std::string(resolutions[resolution_]);
        case 2: return std::string("FLOOR AND CEILING : ")+(planes_?"ON":"OFF");
        case 3: return std::string("COLORED LIGHTS : ")+(coloredLights_?"ON":"OFF");
        default: return "BACK";
        }
    case PortMenuPage::Audio:
        return row==0?"MASTER VOLUME : "+std::to_string(volume_):"BACK";
    case PortMenuPage::Controls:
        return row==0?std::string("CONTROL SCHEME : ")+(modernControls_?"MODERN":"CLASSIC"):"BACK";
    case PortMenuPage::Language:
        return row==3?"BACK":std::string(languageLabels[row])+(row==language_?" - SELECTED":"");
    }
    return "";
}

void PortMenuState::back()
{
    page_=page_==PortMenuPage::Options?PortMenuPage::Main:PortMenuPage::Options;
    selection_=0;
}

void PortMenuState::changeValue(int delta)
{
    switch(page_) {
    case PortMenuPage::Video:
        switch(selection_) {
        case 0: aspect_=unsigned((int(aspect_)+delta+int(aspectValues.size()))%int(aspectValues.size())); break;
        case 1: resolution_=unsigned((int(resolution_)+delta+int(resolutions.size()))%int(resolutions.size())); break;
        case 2: planes_=!planes_; break;
        case 3: coloredLights_=!coloredLights_; break;
        }
        break;
    case PortMenuPage::Audio:
        if(selection_==0) volume_=unsigned(std::clamp(int(volume_)+delta*10,0,100));
        break;
    case PortMenuPage::Controls:
        if(selection_==0) modernControls_=!modernControls_;
        break;
    case PortMenuPage::Language:
        if(selection_<3) language_=selection_;
        break;
    default: break;
    }
}

PortMenuChoice PortMenuState::activate()
{
    switch(page_) {
    case PortMenuPage::Main:
        if(selection_==0) return PortMenuChoice::NewGame;
        if(selection_==2) {page_=PortMenuPage::Options;selection_=0;}
        if(selection_==3) return PortMenuChoice::Exit;
        break; // LOAD GAME is visible and selectable, not operational.
    case PortMenuPage::Options:
        if(selection_==4) back();
        else {page_=static_cast<PortMenuPage>(int(PortMenuPage::Video)+int(selection_));selection_=0;}
        break;
    case PortMenuPage::Video:
        if(selection_==4) back(); else changeValue(1);
        break;
    case PortMenuPage::Audio: case PortMenuPage::Controls:
        if(selection_==1) back(); else changeValue(1);
        break;
    case PortMenuPage::Language:
        if(selection_==3) back(); else changeValue(1);
        break;
    }
    return PortMenuChoice::None;
}

int PortMenuState::hitTest(float x,float y,unsigned count,bool submenu)
{
    const float left=submenu?submenuLeft:menuLeft;
    const float right=submenu?submenuRight:menuRight;
    if (x<left || x>right) return -1;
    for (unsigned i=0;i<count;++i) {
        const float top=rowTop(i,count,submenu);
        if (y>=top && y<top+rowHeight) return int(i);
    }
    return -1;
}

bool PortMenuState::pointerMove(float x,float y)
{
    const int hit=hitTest(x,y,rowCount(),page_!=PortMenuPage::Main);
    if (hit<0 || unsigned(hit)==selection_) return false;
    selection_=unsigned(hit);
    return true;
}

PortMenuChoice PortMenuState::pointerClick(float x,float y)
{
    const int hit=hitTest(x,y,rowCount(),page_!=PortMenuPage::Main);
    if (hit<0) return PortMenuChoice::None;
    selection_=unsigned(hit);
    return activate();
}

PortMenuChoice PortMenuState::key(SDL_Scancode pressed,bool repeated)
{
    if (repeated) return PortMenuChoice::None;
    if (pressed==SDL_SCANCODE_ESCAPE || pressed==SDL_SCANCODE_BACKSPACE) {
        if (page_!=PortMenuPage::Main) {back(); return PortMenuChoice::None;}
        if(pressed==SDL_SCANCODE_ESCAPE && inGame_) return PortMenuChoice::Resume;
        return PortMenuChoice::None;
    }
    if (pressed==SDL_SCANCODE_UP || pressed==SDL_SCANCODE_W)
        selection_=(selection_+rowCount()-1)%rowCount();
    else if (pressed==SDL_SCANCODE_DOWN || pressed==SDL_SCANCODE_S)
        selection_=(selection_+1)%rowCount();
    else if (pressed==SDL_SCANCODE_LEFT || pressed==SDL_SCANCODE_A)
        changeValue(-1);
    else if (pressed==SDL_SCANCODE_RIGHT || pressed==SDL_SCANCODE_D)
        changeValue(1);
    else if (pressed==SDL_SCANCODE_RETURN || pressed==SDL_SCANCODE_KP_ENTER ||
             pressed==SDL_SCANCODE_SPACE || pressed==SDL_SCANCODE_E)
        return activate();
    return PortMenuChoice::None;
}

PortMenuChoice showPortMenu(SDL_Window* window,SDL_Renderer* renderer,
                            const std::filesystem::path& dataDirectory,bool inGame)
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
    MenuAudio audio(dataDirectory);
    const auto windowId=SDL_GetWindowID(window);
    PortMenuState state(inGame);
    double seconds=0.,selectionChangedAt=0.;
    const std::uint32_t effectSeed=hash32(std::uint32_t(SDL_GetTicksNS()));
    std::uint32_t selectionSeed=hash32(effectSeed ^ 0x51409a6du);
    unsigned selectionCount=0;
    // Comparing displayed option values catches keys, mouse clicks and language
    // selection without adding settings or real effects to the gameplay model.
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
        const bool changedValue=!changedPage && valuesOnPage()!=previousValues;
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
            if (!focused) continue;
            if (event.type==SDL_EVENT_KEY_DOWN && event.key.windowID==windowId) {
                const auto old=state.selection();
                const auto oldPage=state.page();
                const auto oldValues=valuesOnPage();
                const auto choice=state.key(event.key.scancode,event.key.repeat);
                if (choice!=PortMenuChoice::None) return choice;
                selectionChanged(oldPage,old,oldValues);
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
                }
            }
        }
        const auto now=SDL_GetTicksNS();
        if (focused) seconds+=std::min(double(now-previous)*1e-9,.06);
        previous=now;
        if (!render(renderer,font.get(),state,seconds,seconds-selectionChangedAt,
                    effectSeed,selectionSeed)) {
            SDL_Log("Port menu rendering failed: %s",SDL_GetError());
            return PortMenuChoice::Error;
        }
        SDL_Delay(16);
    }
}
