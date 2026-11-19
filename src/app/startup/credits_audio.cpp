#include "credits_audio.h"
#include "credits_timeline.h"

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_init.h>
#include <cmath>
#include <cstdint>
#include <vector>

namespace startup_credits {
namespace {
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
} // namespace

void CreditsAudio::start()
{
    if (!(SDL_WasInit(SDL_INIT_AUDIO)&SDL_INIT_AUDIO)) {
        if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) return;
        initialized=true;
    }
    constexpr int rate=24000;
    const SDL_AudioSpec spec{SDL_AUDIO_F32,1,rate};
    stream=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,nullptr,nullptr);
    if (!stream) return;
    std::vector<float> pcm(std::size_t(duration*rate),0.f);
    for (std::size_t i=0;i<pcm.size();++i) {
        const double t=double(i)/rate;
        const float envelope=fade(t,.6f)*(1.f-fade(t-(duration-1.1),1.1f));
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

CreditsAudio::~CreditsAudio()
{
    if (stream) SDL_DestroyAudioStream(stream);
    if (initialized) SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void CreditsAudio::setFocused(bool focused)
{
    if (!stream) return;
    if (focused) SDL_ResumeAudioStreamDevice(stream);
    else SDL_PauseAudioStreamDevice(stream);
}
} // namespace startup_credits
