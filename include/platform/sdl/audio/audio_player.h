#pragma once

#include "extremis/audio/audio_mixer.h"
#include "vortex/audio/sdl_player.h"

namespace audio {
class Player {
public:
    bool open(const std::filesystem::path& bank, std::string& error, bool single = false);
    void close() { player_.close(); }
    bool play(std::size_t id, bool repeat = false, Playback playback = Playback::Native);
    void stop(std::size_t id) { player_.stop(id); }
    void setVolume(float volume) { player_.setVolume(volume); }
    void pause(bool paused) { player_.pause(paused); }
    std::size_t effectCount() const { return player_.sampleCount(); }
    std::size_t activeVoices() { return player_.activeVoices(); }
    bool playing(std::size_t id) { return player_.playing(id); }
    std::optional<PlaybackStats> statistics(std::size_t id) { return player_.statistics(id); }

private:
    vortex::audio::Player player_{mixerConfig};
};
} // namespace audio
