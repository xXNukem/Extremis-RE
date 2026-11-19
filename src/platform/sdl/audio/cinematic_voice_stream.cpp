#include "platform/sdl/audio/cinematic_voice_stream.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <utility>

namespace platform::sdl {

CinematicVoiceStream::CinematicVoiceStream()
    : mixers_{vortex::audio::Mixer(audio::mixerConfig), vortex::audio::Mixer(audio::mixerConfig)}
{
}

CinematicVoiceStream::~CinematicVoiceStream()
{
    if (stream_) {
        SDL_SetAudioStreamGetCallback(stream_, nullptr, nullptr);
        SDL_DestroyAudioStream(stream_);
    }
    if (initialized_) SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void CinematicVoiceStream::open(audio::Bank bank, const char* title)
{
    auto samples = audio::mixerSamples(std::move(bank));
    mixers_[0].setSamples(samples);
    mixers_[1].setSamples(std::move(samples));
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        SDL_Log("%s audio: %s", title, SDL_GetError());
        return;
    }
    initialized_ = true;
    const SDL_AudioSpec spec{SDL_AUDIO_F32, 1, audio::sampleRate};
    stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, feed, this);
    if (!stream_ || !SDL_ResumeAudioStreamDevice(stream_)) {
        SDL_Log("%s audio device: %s", title, SDL_GetError());
        if (stream_) SDL_DestroyAudioStream(stream_);
        stream_ = nullptr;
    }
}

bool CinematicVoiceStream::lock()
{
    return stream_ && SDL_LockAudioStream(stream_);
}

void CinematicVoiceStream::unlock()
{
    SDL_UnlockAudioStream(stream_);
}

void CinematicVoiceStream::setCue(unsigned voice, unsigned sound, unsigned volume)
{
    auto& mixer = mixers_[voice];
    mixer.clear();
    if (sound) {
        auto options = audio::playbackOptions(false, audio::Playback::Native);
        options.gain = static_cast<std::uint8_t>(volume) / 64.0f;
        mixer.play(sound, options);
    }
}

bool CinematicVoiceStream::voice0Active()
{
    if (!lock()) return false;
    const bool active = mixers_[0].active() != 0;
    unlock();
    return active;
}

void CinematicVoiceStream::pause(bool paused)
{
    if (stream_) {
        if (paused) SDL_PauseAudioStreamDevice(stream_);
        else SDL_ResumeAudioStreamDevice(stream_);
    }
}

void SDLCALL CinematicVoiceStream::feed(void* user, SDL_AudioStream* stream, int additional, int)
{
    auto& self = *static_cast<CinematicVoiceStream*>(user);
    std::array<float, 512> a{}, b{};
    while (additional > 0) {
        const int frames = std::min<int>(512, (additional - 1) / sizeof(float) + 1);
        self.mixers_[0].render(a.data(), frames);
        self.mixers_[1].render(b.data(), frames);
        for (int i = 0; i < frames; ++i) a[i] = std::clamp(a[i] + b[i], -1.f, 1.f);
        if (!SDL_PutAudioStreamData(stream, a.data(), frames * int(sizeof(float)))) return;
        additional -= frames * int(sizeof(float));
    }
}

} // namespace platform::sdl
