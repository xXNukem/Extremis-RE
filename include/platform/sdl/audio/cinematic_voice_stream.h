#pragma once

#include "extremis/audio/audio_mixer.h"

#include <SDL3/SDL_audio.h>
#include <array>
#include <vector>

namespace platform::sdl {

// Owns two independently cued voices mixed to one mono SDL stream.
class CinematicVoiceStream {
public:
    CinematicVoiceStream();
    ~CinematicVoiceStream();
    CinematicVoiceStream(const CinematicVoiceStream&) = delete;
    CinematicVoiceStream& operator=(const CinematicVoiceStream&) = delete;

    // Index zero is the stop sentinel. The caller opens the stream once.
    void open(audio::Bank bank, const char* title);
    void pause(bool paused);
    bool voice0Active();

    // Cue carries voice (0 or 1), sound (an index into the bank) and volume.
    // A batch is applied under one lock.
    template<class Cue>
    void cues(const std::vector<Cue>& cues);

private:
    bool lock();
    void unlock();
    void setCue(unsigned voice, unsigned sound, unsigned volume);
    static void SDLCALL feed(void* user, SDL_AudioStream* stream, int additional, int total);

    std::array<vortex::audio::Mixer, 2> mixers_;
    SDL_AudioStream* stream_ = nullptr;
    bool initialized_ = false;
};

} // namespace platform::sdl

#include "platform/sdl/audio/cinematic_voice_stream.tpp"
