#pragma once

#include "vortex/audio/mixer.h"
#include <string>
#include <SDL3/SDL_audio.h>

namespace vortex::audio {
class Player {
public:
    explicit Player(MixerConfig config);
    ~Player();
    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;
    // Lifecycle and control calls are serialized by the caller. SDL synchronizes
    // the producer callback with control calls while the device is open.
    bool open(int rate, std::vector<Sample> samples, std::string& error);
    void close();
    bool play(std::size_t id, PlayOptions options = {});
    void stop(std::size_t id);
    void setVolume(float volume);
    void pause(bool paused);
    std::size_t sampleCount() const;
    std::size_t activeVoices();
    bool playing(std::size_t id);
    std::optional<PlaybackStats> statistics(std::size_t id);
private:
    static void SDLCALL feed(void* userdata, SDL_AudioStream* stream, int additional, int total);
    Mixer mixer_;
    SDL_AudioStream* stream_ = nullptr;
    bool initialized_ = false;
};
} // namespace vortex::audio
