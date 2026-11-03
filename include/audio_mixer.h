#pragma once

#include "audio_data.h"
#include <array>
#include <optional>

namespace audio {
enum class Playback { Native, Once };
struct PlaybackStats {
    std::size_t decodedFrames = 0;
    std::uint64_t requests = 0, started = 0, sourceFrames = 0;
    std::uint64_t wraps = 0, completed = 0, stopped = 0;
};

class Mixer {
public:
    explicit Mixer(const Bank& bank);
    bool play(std::size_t id, bool repeat = false, Playback playback = Playback::Native);
    void stop(std::size_t id);
    void clear();
    void setVolume(float volume);
    std::size_t active() const;
    bool playing(std::size_t id) const;
    void resetStatistics(); // Call while idle after replacing the referenced bank.
    std::optional<PlaybackStats> statistics(std::size_t id) const;
    void render(float* output, std::size_t frames);

private:
    struct Voice {
        const Effect* effect = nullptr;
        std::size_t id = 0, cursor = 0;
        float envelope = 0;
        bool repeat = false, releasing = false, inLoop = false, sampleLoop = true;
    };
    float next(Voice& voice);
    const Bank& bank_;
    std::array<Voice,4> voices_{};
    std::vector<PlaybackStats> statistics_;
    float volume_ = 0.6f; // Port master level, adjustable; not an original configuration value.
};
} // namespace audio
