#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace vortex::audio {

struct Loop {
    std::size_t begin;
    std::size_t end;      // Exclusive boundary of subsequent passes.
    std::size_t firstEnd; // Exclusive boundary of the initial pass, from frame zero.
};

struct Sample {
    // Mono float PCM at the consumer's common source rate.
    std::vector<float> pcm;
    float gain = 1.0f;
    std::optional<Loop> loop;
};

struct MixerConfig {
    std::size_t voiceCount;
    unsigned rampFrames; // Zero disables the fade; a stop then emits silence immediately.
    float masterVolume = 1.0f;
};

struct PlayOptions {
    bool repeat = false; // Repeat the whole sample when no sample loop is used.
    bool useSampleLoop = true;
    bool restartExisting = false;
    std::optional<float> gain; // Overrides the sample gain for this voice.
};

struct PlaybackStats {
    std::size_t decodedFrames = 0;
    std::uint64_t requests = 0, started = 0, sourceFrames = 0;
    std::uint64_t wraps = 0, completed = 0, stopped = 0;
};

class Mixer {
public:
    explicit Mixer(MixerConfig config);
    Mixer(const Mixer&) = delete;
    Mixer& operator=(const Mixer&) = delete;
    // Replacing the samples clears voices and resets statistics.
    void setSamples(std::vector<Sample> samples);
    bool play(std::size_t id, PlayOptions options = {});
    void stop(std::size_t id);
    void clear();
    void setVolume(float volume);
    std::size_t sampleCount() const;
    std::size_t active() const;
    bool playing(std::size_t id) const;
    std::optional<PlaybackStats> statistics(std::size_t id) const;
    void render(float* output, std::size_t frames);

private:
    struct Voice {
        const float* pcm = nullptr;
        PlaybackStats* stats = nullptr;
        const Loop* loop = nullptr;
        std::size_t id = 0, cursor = 0, boundary = 0;
        unsigned envelopeFrames = 0;
        float gain = 1.0f;
        bool looping = false, releasing = false;
    };
    float next(Voice& voice);

    std::vector<Sample> samples_;
    std::vector<Voice> voices_;
    std::vector<PlaybackStats> statistics_;
    float volume_ = 1.0f;
    unsigned rampFrames_;
    float ramp_;
    float normalization_;
};

} // namespace vortex::audio
