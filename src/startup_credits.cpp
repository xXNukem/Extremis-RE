#include "startup_credits.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace {
constexpr float width=640.f, height=400.f;
constexpr double duration=26.5, firstEnd=7.4, secondEnd=16.5, titleStart=21.1;
constexpr double pi=3.14159265358979323846;

// One distinct left-to-right sweep per tracker ping. The last three sweeps
// deteriorate sharply; the title phase never draws a waveform.
struct Sweep { double at, travel; float damage; unsigned seed; };
constexpr std::array<Sweep,15> sweeps{{
    {.65,1.10,.08f, 0}, {2.48,1.08,.13f, 1}, {4.32,1.03,.15f, 2}, {6.13,.94,.22f, 3},
    {7.76,.97,.28f, 4}, {9.18,.92,.34f, 5}, {10.48,.86,.39f, 6},
    {11.69,.82,.49f, 7}, {12.86,.76,.56f, 8}, {13.93,.72,.65f, 9},
    {15.01,.72,.70f,10},
    {16.69,.89,.84f,11}, {17.65,.78,.92f,12},
    {18.51,.71,.98f,13}, {19.31,.65,1.00f,14}
}};

float fade(double t, double seconds)
{
    const double v=std::clamp(t/seconds,0.0,1.0);
    return float(v*v*(3.0-2.0*v));
}

Uint8 opacity(float value) { return Uint8(std::clamp(value,0.f,1.f)*255.f); }

// The monitor is always black. Only the trace changes colour as the
// life-support signal becomes progressively less viable.
struct Rgb { Uint8 red, green, blue; };
Rgb ecgTint(float damage)
{
    struct Stop { float position; Rgb color; };
    constexpr std::array<Stop,5> palette{{
        {0.f, {124,255,158}},  // Healthy: green phosphor.
        {.32f,{163,255, 91}}, // Weakening: yellow-green.
        {.62f,{255,162, 45}}, // Critical: amber.
        {.83f,{255, 47, 54}}, // Near death: red.
        {1.f, {255, 23, 42}}
    }};
    const float progress=std::clamp((damage-.08f)/.92f,0.f,1.f);
    for (std::size_t i=1;i<palette.size();++i) {
        if (progress>palette[i].position) continue;
        const auto& from=palette[i-1], &to=palette[i];
        const float blend=(progress-from.position)/(to.position-from.position);
        const auto mix=[blend](Uint8 a,Uint8 b) {
            return Uint8(std::lround(float(a)+(float(b)-float(a))*blend));
        };
        return {mix(from.color.red,to.color.red),mix(from.color.green,to.color.green),
                mix(from.color.blue,to.color.blue)};
    }
    return palette.back().color;
}

bool color(SDL_Renderer* r, Uint8 red, Uint8 green, Uint8 blue, float alpha=1.f)
{
    return SDL_SetRenderDrawColor(r,red,green,blue,opacity(alpha));
}

bool centered(SDL_Renderer* r, const std::string& s, float y, float scale,
              Uint8 red, Uint8 green, Uint8 blue, float alpha=1.f, float shift=0.f)
{
    if (!SDL_SetRenderScale(r,scale,scale)) return false;
    const float x=(width-float(s.size())*8.f*scale)*.5f+shift;
    const bool ok=color(r,red,green,blue,alpha) &&
                  SDL_RenderDebugText(r,x/scale,y/scale,s.c_str());
    return SDL_SetRenderScale(r,1.f,1.f) && ok;
}

// Catastrophic CRT synchronization faults: blackouts, separated ghost images
// and short, torn horizontal streaks. These affect TEXT only; the screen
// behind the letters remains pure black.
struct TextFault {
    bool active, blackout;
    float displacement, intensity;
    unsigned phase;
};

TextFault textFault(double time, float severity)
{
    const unsigned tick=unsigned(std::floor(time*31.0));
    const bool fault=(tick%59>=9 && tick%59<=14) ||
                     (tick%47>=27 && tick%47<=30) ||
                     (tick%101>=63 && tick%101<=68);
    const bool blackout=(tick%59==12 || tick%47==29 || tick%101==65);
    const float displacement=fault?float(int(tick*37%21)-10)*severity:0.f;
    const float intensity=blackout?.09f:(fault?.51f:1.f);
    return {fault,blackout,displacement,intensity,tick};
}

bool unstableText(SDL_Renderer* r, std::string_view text, float x, float y, float scale,
                  Uint8 red, Uint8 green, Uint8 blue, float alpha, double time,
                  float severity=1.f)
{
    if (text.empty() || alpha<=0.f) return true;
    const auto fault=textFault(time,severity);
    const std::string copy(text);
    if (!SDL_SetRenderScale(r,scale,scale)) return false;
    bool ok=true;
    if (fault.active) {
        // Strongly displaced phosphor after-images, briefly splitting letters.
        ok=color(r,42,255,115,alpha*.45f) &&
           SDL_RenderDebugText(r,(x-fault.displacement*1.6f)/scale,(y+3.f)/scale,copy.c_str()) &&
           color(r,157,255,188,alpha*.33f) &&
           SDL_RenderDebugText(r,(x+fault.displacement+8.f)/scale,(y-2.f)/scale,copy.c_str());
    }
    if (ok) {
        ok=color(r,red,green,blue,alpha*fault.intensity) &&
           SDL_RenderDebugText(r,(x+fault.displacement)/scale,y/scale,copy.c_str());
    }
    if (!SDL_SetRenderScale(r,1.f,1.f) || !ok) return false;
    if (fault.active) {
        // Thin horizontal tears confined to the text itself, not the backdrop.
        const float span=float(copy.size())*8.f*scale;
        for (unsigned i=0;i<3;++i) {
            const float row=y+2.f+float((fault.phase*13u+i*17u)%unsigned(std::max(2.f,8.f*scale)));
            const float left=x+float((fault.phase*29u+i*41u)%unsigned(std::max(2.f,span*.45f)));
            const float right=std::min(x+span,left+span*(.16f+.11f*float(i)));
            if (!color(r,54,255,126,alpha*(fault.blackout?.28f:.38f)) ||
                !SDL_RenderLine(r,left,row,right,row)) return false;
        }
    }
    return true;
}

bool unstableCentered(SDL_Renderer* r, const std::string& text, float y, float scale,
                      Uint8 red, Uint8 green, Uint8 blue, float alpha, double time,
                      float severity=1.f)
{
    const float x=(width-float(text.size())*8.f*scale)*.5f;
    return unstableText(r,text,x,y,scale,red,green,blue,alpha,time,severity);
}

// The original game's lettering is stored in CASQUE.SPR. The startup function
// has no asset path; preserve the established title instead of changing main.
bool accentOnPerez(SDL_Renderer* r, float y, float scale, float alpha)
{
    constexpr std::string_view name="Carlos (xXNukem) de la Barrera Perez";
    const float x=(width-float(name.size())*8.f*scale)*.5f+
                  float(name.find("Perez")+1)*8.f*scale+4.f*scale;
    return color(r,195,255,209,alpha) && SDL_RenderLine(r,x-2,y-3,x+1,y-7);
}

float triangle(float x, float center, float halfWidth)
{
    return std::max(0.f,1.f-std::abs(x-center)/halfWidth);
}

// Stable, individual P-QRS-T shapes: each sweep has a different position,
// amplitude, number of spikes and amount of electrical disturbance.
float waveform(float x, const Sweep& beat)
{
    const float d=beat.damage;
    const float variant=float(beat.seed);
    const float qrsX=286.f+17.f*std::sin(variant*1.72f);
    const float qrsWidth=8.f+d*8.f+float(beat.seed%3)*2.f;
    float signal=.12f*triangle(x,113.f+11.f*std::sin(variant),36.f)
                 -.19f*triangle(x,qrsX-30.f,23.f)
                 +(1.04f-.15f*d)*triangle(x,qrsX,qrsWidth)
                 -(.69f+.15f*d)*triangle(x,qrsX+23.f,22.f)
                 +(.24f+.06f*std::sin(variant*2.2f))*triangle(x,460.f-variant*2.4f,53.f);
    if (beat.seed%4==1) signal+=.20f*triangle(x,492.f,15.f);
    if (beat.seed%4==2) signal-=.23f*triangle(x,376.f,20.f);
    if (beat.seed%4==3) signal+=.45f*d*triangle(x,399.f,10.f)
                                 -.32f*d*triangle(x,424.f,14.f);
    if (d>.60f) {
        const float chaos=(d-.60f)/.40f;
        signal+=chaos*(.15f*std::sin(x*.23f+variant*1.7f)
                      +.09f*std::sin(x*.61f+variant*.9f));
        signal+=chaos*(.55f*triangle(x,380.f+variant*2.f,9.f)
                      -.36f*triangle(x,402.f+variant,14.f));
        if (beat.seed%3==0) signal*=(1.f-.55f*chaos); // Failing, weak QRS.
        if (beat.seed%3==2) signal=-signal*.78f;      // Ectopic inversion.
    }
    const float drift=.018f*std::sin(x*.026f+variant*1.1f);
    return signal+drift;
}

float textFlicker(double time)
{
    const int tick=int(std::floor(time*23.0));
    const bool dropout=(tick%53>=12 && tick%53<=13) ||
                       (tick%37==17) || (tick%67>=39 && tick%67<=41);
    const float brightness=dropout?.12f:(.82f+.18f*std::sin(float(time)*49.f));
    // Keep flickering even while the name is being typed; there is no
    // "stabilize after reveal" condition.
    return brightness*(.78f+.22f*fade(time,.7));
}

bool blackBackground(SDL_Renderer* r)
{
    return color(r,0,0,0) && SDL_RenderClear(r);
}

bool phosphorSweep(SDL_Renderer* r, const Sweep& beat, double now)
{
    const double age=now-beat.at;
    if (age<0.0 || age>=beat.travel+.36) return true;
    const float head=std::clamp(float(age/beat.travel)*width,0.f,width);
    if (head<2.f) return true;
    const float after=(1.f-fade(age-beat.travel,.36));
    const float flare=fade(age,.065)*after;
    const float gain=(1.f+.16f*beat.damage)*flare;
    const float amplitude=124.f+35.f*beat.damage;
    const auto tint=ecgTint(beat.damage);

    // Reveal the signal progressively from left to right. Each 64-pixel
    // piece has an individual phosphor afterglow, fading behind the head.
    // No continuous baseline or trace survives between sweeps.
    std::array<SDL_FPoint,65> points{};
    for (int chunk=0; chunk<10; ++chunk) {
        const int begin=chunk*64, end=std::min(640,int(head));
        if (end<=begin) break;
        const int stop=std::min(begin+64,end);
        if (beat.damage>.8f && (unsigned(chunk)+beat.seed*3)%17==0) continue;
        const float midpoint=float(begin+stop)*.5f;
        const float distance=std::max(0.f,(head-midpoint)/width);
        const float afterglow=.22f+.78f*std::exp(-3.5f*distance);
        const float brightness=gain*afterglow;
        if (brightness<.02f) continue;
        for (int pass=0; pass<5; ++pass) {
            const float offset=pass==0?-7.f:pass==1?7.f:pass==2?-2.5f:pass==3?2.5f:0.f;
            const float strength=pass<2?.08f:pass<4?.22f:1.f;
            const float bleed=pass<2?.52f:pass<4?.85f:1.f;
            if (!color(r,Uint8(tint.red*bleed),Uint8(tint.green*bleed),
                       Uint8(tint.blue*bleed),brightness*strength)) return false;
            const int n=stop-begin+1;
            for (int j=0; j<n; ++j) {
                const float x=float(begin+j);
                points[std::size_t(j)]={x,207.f-amplitude*waveform(x,beat)+offset};
            }
            if (!SDL_RenderLines(r,points.data(),n)) return false;
        }
    }
    // No fullscreen flashes: the beam alone emits light against pure black.
    return true;
}

bool drawECG(SDL_Renderer* r, double time)
{
    if (time>=titleStart) return true; // The red logo has NO ECG underneath.
    for (const auto& beat:sweeps)
        if (!phosphorSweep(r,beat,time)) return false;
    return true;
}

bool drawCredit(SDL_Renderer* r, double time)
{
    if (!blackBackground(r) || !drawECG(r,time)) return false;
    if (time<firstEnd) {
        const float appear=fade(time-1.05,1.1);
        const float visible=appear*(1.f-fade(time-(firstEnd-1.45),1.25));
        if (!unstableCentered(r,"Cyanide Software Presents",181,2.f,
                            178,255,196,visible*textFlicker(time),time,.90f)) return false;
    } else if (time<secondEnd) {
        const double local=time-firstEnd;
        const float appear=fade(local-.90,1.1);
        const float visible=appear*(1.f-fade(time-(secondEnd-1.50),1.30));
        const float intensity=visible*textFlicker(time);
        if (!unstableCentered(r,"A source port by",154,2.f,175,252,198,intensity,time,1.25f)) return false;
        constexpr std::string_view name="Carlos (xXNukem) de la Barrera Perez";
        const float scale=1.5f;
        const unsigned count=unsigned(std::clamp((local-1.55)*8.2,0.0,double(name.size())));
        const float origin=(width-float(name.size())*8.f*scale)*.5f;
        const std::string typed(name.substr(0,count));
        // The same violent glitches continue THROUGHOUT the typewriter reveal.
        if (!unstableText(r,typed,origin,207.f,scale,192,255,212,
                          intensity,time,1.35f)) return false;
        if (count>name.find("Perez")+1 && !accentOnPerez(r,207.f,scale,intensity)) return false;
        if (count<name.size() && count>0 && std::fmod(local*1.5,1.0)<.50) {
            const float caret=origin+float(count)*8.f*scale;
            if (!color(r,145,255,177,intensity) ||
                !SDL_RenderLine(r,caret,207.f,caret,219.f)) return false;
        }
    } else if (time>=titleStart) {
        const double local=time-titleStart;
        const float ignite=fade(local-.25,.95);
        const float visible=ignite*(1.f-fade(time-(duration-1.65),1.45));
        const auto fault=textFault(time,1.15f);
        const float flicker=local<1.2
            ? (.66f+.34f*std::sin(float(local)*54.f))*fault.intensity
            : fault.intensity;
        const float shift=(local<.35?1.2f*std::sin(float(local)*69.f):0.f)+
                          fault.displacement;
        // The title's red letter glow and bars are the only lit details.
        if (!color(r,220,24,48,visible*.19f)) return false;
        for (const float y:{163.f,167.f,237.f,241.f})
            if (!SDL_RenderLine(r,120.f,y,520.f,y)) return false;
        if (!color(r,239,39,57,visible*.94f) ||
            !SDL_RenderLine(r,128.f,165.f,512.f,165.f) ||
            !SDL_RenderLine(r,128.f,239.f,512.f,239.f)) return false;
        if (!centered(r,"EXTREMIS RE",184.f,4.f,188,24,37,visible*.35f,shift-2.f) ||
            !centered(r,"EXTREMIS RE",184.f,4.f,188,24,37,visible*.35f,shift+2.f) ||
            !centered(r,"EXTREMIS RE",184.f,4.f,252,69,76,visible*flicker,shift)) return false;
        // Red tearing on the title, without compromising the recognizable logo.
        if (fault.active && !color(r,255,57,68,visible*.46f)) return false;
        if (fault.active) {
            const float left=138.f+float(fault.phase*19u%100u);
            if (!SDL_RenderLine(r,left,195.f,left+260.f,195.f) ||
                !SDL_RenderLine(r,left+30.f,223.f,left+165.f,223.f)) return false;
        }
    }
    // Between the second credit and the title: only the dying ECG sweeps.
    return SDL_RenderPresent(r);
}

// Original, synthesized short tracker-like ping. No extracted film samples.
double trackerChirp(double t, float damage)
{
    if (t<0.0 || t>.26) return 0.0;
    const double high=1450.0+damage*210.0, low=680.0+damage*130.0;
    const double phase=2*pi*(low*t+(high-low)*(1.0-std::exp(-29.0*t))/29.0);
    const double envelope=std::min(1.0,t/.003)*std::exp(-t*(16.0+damage*2.0))
                         *(1.0-fade(t-.20,.06));
    const double tone=std::sin(phase+.14*std::sin(2*pi*30.0*t))
                     +.20*std::sin(phase*2.03+.3);
    const double click=.16*std::sin(2*pi*3100.0*t)*std::exp(-t*85.0);
    return envelope*(.090*tone+.02*click);
}

// An original, synthesized horror stinger: detuned metallic shriek over a
// deep, falling reactor-thump, with a coarse initial blast and a long tail.
// No licensed samples, external files or new libraries.
double logoStinger(double t, double grit)
{
    if (t<0.0 || t>=2.85) return 0.0;
    const double attack=std::min(1.0,t/.008);
    const double subPhase=2*pi*(39.0*t+56.0*(1.0-std::exp(-4.4*t))/4.4);
    const double punchPhase=2*pi*(60.0*t+115.0*(1.0-std::exp(-18.0*t))/18.0);
    const double sub=.32*std::sin(subPhase)*std::exp(-1.30*t)*attack;
    const double punch=.26*std::sin(punchPhase)*std::exp(-5.1*t)*attack;
    const double metal=(.12*std::sin(2*pi*181.0*t+.45*std::sin(2*pi*9.0*t))+
                        .13*std::sin(2*pi*259.0*t)+
                        .075*std::sin(2*pi*307.0*t))*std::exp(-2.0*t)*attack;
    const double blast=.23*grit*std::exp(-19.0*t)+
                       .045*grit*std::exp(-2.4*t);
    return sub+punch+metal+blast;
}

struct IntroAudio {
    SDL_AudioStream* stream=nullptr;
    bool initialized=false;

    void start()
    {
        if (!(SDL_WasInit(SDL_INIT_AUDIO)&SDL_INIT_AUDIO)) {
            if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) return; // Silent fallback.
            initialized=true;
        }
        constexpr int rate=24000;
        const SDL_AudioSpec spec{SDL_AUDIO_F32,1,rate};
        stream=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,nullptr,nullptr);
        if (!stream) return;
        std::vector<float> pcm(std::size_t(duration*rate),0.f);
        for (std::size_t i=0;i<pcm.size();++i) {
            const double t=double(i)/rate;
            const float envelope=fade(t,.6)*(1.f-fade(t-(duration-1.4),1.4));
            pcm[i]=envelope*float(.006*std::sin(2*pi*53*t)+.003*std::sin(2*pi*71*t));
        }
        for (const auto& beat:sweeps) {
            const std::size_t start=std::size_t(beat.at*rate);
            for (int j=0;j<int(rate*.44) && start+std::size_t(j)<pcm.size();++j) {
                const double t=double(j)/rate;
                const double ping=trackerChirp(t,beat.damage)
                                 +.28*trackerChirp(t-.095,beat.damage)
                                 +.12*trackerChirp(t-.188,beat.damage);
                const float volume=1.f+.14f*beat.damage;
                pcm[start+std::size_t(j)]+=float(volume*ping);
            }
        }
        // Silent typewriter. A far heavier, dissonant impact hits exactly
        // when the red title starts emerging from black.
        const std::size_t titleSample=std::size_t((titleStart+.25)*rate);
        std::uint32_t random=0xFA71FACEu;
        double lowNoise=0.;
        for (int j=0;j<int(rate*2.85) && titleSample+std::size_t(j)<pcm.size();++j) {
            const double t=double(j)/rate;
            random=random*1664525u+1013904223u;
            const double noise=double((random>>8)&0xFFFFu)/32768.0-1.0;
            lowNoise+=.13*(noise-lowNoise);
            pcm[titleSample+std::size_t(j)]+=float(logoStinger(t,noise-lowNoise));
        }
        for (float& sample:pcm) sample=std::clamp(sample,-.75f,.75f);
        if (!SDL_PutAudioStreamData(stream,pcm.data(),int(pcm.size()*sizeof(float))) ||
            !SDL_ResumeAudioStreamDevice(stream)) {
            SDL_DestroyAudioStream(stream);
            stream=nullptr;
        }
    }

    ~IntroAudio()
    {
        if (stream) SDL_DestroyAudioStream(stream);
        if (initialized) SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
};
} // namespace

StartupCreditsResult showStartupCredits(SDL_Window* window, SDL_Renderer* renderer)
{
    int oldWidth=0,oldHeight=0;
    float oldScaleX=1.f,oldScaleY=1.f;
    Uint8 oldR=0,oldG=0,oldB=0,oldA=255;
    SDL_RendererLogicalPresentation oldMode;
    SDL_BlendMode oldBlend;
    if (!SDL_GetRenderLogicalPresentation(renderer,&oldWidth,&oldHeight,&oldMode) ||
        !SDL_GetRenderScale(renderer,&oldScaleX,&oldScaleY) ||
        !SDL_GetRenderDrawColor(renderer,&oldR,&oldG,&oldB,&oldA) ||
        !SDL_GetRenderDrawBlendMode(renderer,&oldBlend) ||
        !SDL_SetRenderLogicalPresentation(renderer,640,400,SDL_LOGICAL_PRESENTATION_LETTERBOX))
        return StartupCreditsResult::Error;

    auto result=StartupCreditsResult::Continue;
    if (!SDL_SetRenderScale(renderer,1.f,1.f) ||
        !SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND))
        result=StartupCreditsResult::Error;

    IntroAudio audio;
    if (result==StartupCreditsResult::Continue) audio.start();

    const auto windowId=SDL_GetWindowID(window);
    auto previous=SDL_GetTicksNS();
    double elapsed=0;
    bool focused=(SDL_GetWindowFlags(window)&SDL_WINDOW_INPUT_FOCUS)!=0;
    if (!focused && audio.stream) SDL_PauseAudioStreamDevice(audio.stream);
    while (result==StartupCreditsResult::Continue && elapsed<duration) {
        SDL_Event event;
        bool done=false;
        while (SDL_PollEvent(&event)) {
            if (event.type==SDL_EVENT_QUIT ||
                (event.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID==windowId)) {
                result=StartupCreditsResult::Quit;
                done=true;
            } else if (event.type==SDL_EVENT_KEY_DOWN && event.key.windowID==windowId && !event.key.repeat) {
                done=true;
            } else if (event.type==SDL_EVENT_WINDOW_FOCUS_LOST && event.window.windowID==windowId) {
                focused=false;
                if (audio.stream) SDL_PauseAudioStreamDevice(audio.stream);
            } else if (event.type==SDL_EVENT_WINDOW_FOCUS_GAINED && event.window.windowID==windowId) {
                focused=true;
                if (audio.stream) SDL_ResumeAudioStreamDevice(audio.stream);
            }
        }
        if (done) break;
        const auto now=SDL_GetTicksNS();
        if (focused) elapsed+=double(now-previous)/1e9;
        previous=now;
        if (elapsed>=duration) break;
        if (!drawCredit(renderer,elapsed)) {
            result=StartupCreditsResult::Error;
            break;
        }
        SDL_Delay(16);
    }

    SDL_ResetKeyboard();
    const bool scaleOK=SDL_SetRenderScale(renderer,oldScaleX,oldScaleY);
    const bool blendOK=SDL_SetRenderDrawBlendMode(renderer,oldBlend);
    const bool colorOK=SDL_SetRenderDrawColor(renderer,oldR,oldG,oldB,oldA);
    const bool modeOK=SDL_SetRenderLogicalPresentation(renderer,oldWidth,oldHeight,oldMode);
    if (!scaleOK || !blendOK || !colorOK || !modeOK)
        result=StartupCreditsResult::Error;
    return result;
}
