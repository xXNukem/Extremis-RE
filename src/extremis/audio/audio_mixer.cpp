#include "extremis/audio/audio_mixer.h"

#include <utility>

namespace audio {
std::vector<vortex::audio::Sample> mixerSamples(Bank bank)
{
    std::vector<vortex::audio::Sample> samples;
    samples.reserve(bank.size());
    for (auto& effect : bank) {
        auto& sample = effect.sample;
        std::optional<vortex::audio::Loop> loop;
        if (sample.loopFlag)
            loop = vortex::audio::Loop{sample.loopBegin, sample.loopEnd, sample.pcm.size()};
        samples.push_back({std::move(sample.pcm), effect.volume / 64.0f, loop});
    }
    return samples;
}

vortex::audio::PlayOptions playbackOptions(bool repeat, Playback playback)
{
    const bool native = playback == Playback::Native;
    return {native && repeat, native, !native, std::nullopt};
}
} // namespace audio
