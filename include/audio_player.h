#pragma once

#include "audio_mixer.h"
#include <SDL3/SDL_audio.h>

namespace audio {
class Player {
public:
    Player();
    ~Player();
    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;
    bool open(const std::filesystem::path& bank, std::string& error, bool single = false);
    void close();
    bool play(std::size_t id, bool repeat = false, Playback playback = Playback::Native);
    void stop(std::size_t id);
    void setVolume(float volume);
    std::size_t effectCount() const;
    std::size_t activeVoices();
    bool playing(std::size_t id);
    std::optional<PlaybackStats> statistics(std::size_t id);
private:
    static void SDLCALL feed(void* userdata, SDL_AudioStream* stream, int additional, int total);
    Bank bank_;
    Mixer mixer_;
    SDL_AudioStream* stream_ = nullptr;
    bool initialized_ = false;
};
} // namespace audio
