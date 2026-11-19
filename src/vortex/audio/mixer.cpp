#include "vortex/audio/mixer.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace vortex::audio {

Mixer::Mixer(MixerConfig config)
    : voices_(config.voiceCount),
      rampFrames_(std::max(1u, config.rampFrames)),
      ramp_(1.0f / float(rampFrames_)),
      normalization_(config.voiceCount ? 1.0f / float(config.voiceCount) : 1.0f)
{
    setVolume(config.masterVolume);
}

void Mixer::setSamples(std::vector<Sample> samples)
{
    std::vector<PlaybackStats> statistics(samples.size());
    for (std::size_t id = 0; id < samples.size(); ++id)
        statistics[id].decodedFrames = samples[id].pcm.size();
    clear();
    samples_ = std::move(samples);
    statistics_ = std::move(statistics);
}

bool Mixer::play(std::size_t id, PlayOptions options)
{
    if (id >= samples_.size() || samples_[id].pcm.empty()) return false;
    ++statistics_[id].requests;
    const auto& sample = samples_[id];
    const auto* loop = sample.loop ? &*sample.loop : nullptr;
    if (loop && (loop->begin >= loop->end || loop->end > sample.pcm.size() ||
                 !loop->firstEnd || loop->firstEnd > sample.pcm.size())) return false;
    if (!options.useSampleLoop) loop = nullptr;

    Voice* target = nullptr;
    if (options.restartExisting)
        for (auto& voice : voices_) if (voice.pcm && voice.id == id) { target = &voice; break; }
    if (!target)
        for (auto& voice : voices_) if (!voice.pcm) { target = &voice; break; }
    if (!target) return false;

    if (target->pcm) ++target->stats->stopped;
    *target = {sample.pcm.data(), &statistics_[id], loop, id, 0, loop ? loop->firstEnd : sample.pcm.size(),
               0, options.gain.value_or(sample.gain), loop || options.repeat, false};
    ++statistics_[id].started;
    return true;
}

void Mixer::stop(std::size_t id)
{
    for (auto& voice : voices_) if (voice.pcm && voice.id == id) voice.releasing = true;
}

void Mixer::clear() { std::fill(voices_.begin(), voices_.end(), Voice{}); }

void Mixer::setVolume(float volume)
{
    if (std::isfinite(volume)) volume_ = std::clamp(volume, 0.0f, 1.0f);
}

std::size_t Mixer::sampleCount() const { return samples_.size(); }

std::size_t Mixer::active() const
{
    return std::count_if(voices_.begin(), voices_.end(), [](const Voice& voice) {
        return voice.pcm != nullptr;
    });
}

bool Mixer::playing(std::size_t id) const
{
    return std::any_of(voices_.begin(), voices_.end(), [id](const Voice& voice) {
        return voice.pcm && voice.id == id;
    });
}

std::optional<PlaybackStats> Mixer::statistics(std::size_t id) const
{
    return id < statistics_.size() ? std::optional<PlaybackStats>(statistics_[id]) : std::nullopt;
}

float Mixer::next(Voice& voice)
{
    if (!voice.pcm) return 0;
    auto& envelope = voice.envelopeFrames;
    if (voice.releasing) envelope -= envelope != 0;
    else envelope += envelope < rampFrames_;
    const auto fadeFrames = voice.looping ? envelope :
                            std::min<std::size_t>(envelope, voice.boundary - voice.cursor);
    const float gain = fadeFrames == rampFrames_ ? 1.0f : float(fadeFrames) * ramp_;
    const float result = voice.pcm[voice.cursor++] * gain * voice.gain;
    ++voice.stats->sourceFrames;
    if (voice.releasing && envelope == 0) {
        ++voice.stats->stopped;
        voice.pcm = nullptr;
    }
    else if (voice.cursor == voice.boundary) {
        if (voice.looping) {
            ++voice.stats->wraps;
            voice.cursor = voice.loop ? voice.loop->begin : 0;
            if (voice.loop) voice.boundary = voice.loop->end;
        }
        else {
            ++voice.stats->completed;
            voice.pcm = nullptr;
        }
    }
    return result;
}

void Mixer::render(float* output, std::size_t frames)
{
    auto* const voices = voices_.data();
    const auto voiceCount = voices_.size();
    for (std::size_t i = 0; i < frames; ++i) {
        float sample = 0;
        for (std::size_t slot = 0; slot < voiceCount; ++slot) sample += next(voices[slot]);
        output[i] = std::clamp(sample * volume_ * normalization_, -1.0f, 1.0f);
    }
}

} // namespace vortex::audio
