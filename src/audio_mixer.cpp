#include "audio_mixer.h"
#include <algorithm>
#include <cmath>

namespace audio {
Mixer::Mixer(const Bank& bank) : bank_(bank) { resetStatistics(); }

bool Mixer::play(std::size_t id, bool repeat, Playback playback)
{
    if (id >= bank_.size() || id >= statistics_.size() || bank_[id].sample.pcm.empty()) return false;
    ++statistics_[id].requests;
    const auto& sample = bank_[id].sample;
    if (sample.rate != 5000 || (sample.loopFlag &&
        (sample.loopBegin >= sample.loopEnd || sample.loopEnd > sample.pcm.size()))) return false;
    // Direct DOS interface requests replace voice 0. For bounded interface
    // effects, reuse the same sample's voice instead of stacking copies.
    Voice* target=nullptr;
    if (playback==Playback::Once)
        for (auto& voice:voices_) if (voice.effect && voice.id==id) { target=&voice; break; }
    if (!target)
        for (auto& voice:voices_) if (!voice.effect) { target=&voice; break; }
    if (target) {
        if (target->effect) ++statistics_[target->id].stopped;
        *target={&bank_[id],id,0,0,repeat && playback==Playback::Native,false,false,playback==Playback::Native};
        ++statistics_[id].started;
        return true;
    }
    return false;
}

void Mixer::stop(std::size_t id)
{
    for (auto& voice : voices_) if (voice.effect && voice.id == id) voice.releasing = true;
}
void Mixer::clear() { voices_ = {}; }
void Mixer::setVolume(float volume)
{
    if (std::isfinite(volume)) volume_ = std::clamp(volume,0.0f,1.0f);
}
std::size_t Mixer::active() const
{
    return std::count_if(voices_.begin(),voices_.end(),[](const Voice& v){ return v.effect != nullptr; });
}

bool Mixer::playing(std::size_t id) const
{
    return std::any_of(voices_.begin(), voices_.end(), [id](const Voice& voice) {
        return voice.effect && voice.id == id;
    });
}

void Mixer::resetStatistics()
{
    clear();
    statistics_.assign(bank_.size(), {});
    for (std::size_t id = 0; id < bank_.size(); ++id)
        statistics_[id].decodedFrames = bank_[id].sample.pcm.size();
}

std::optional<PlaybackStats> Mixer::statistics(std::size_t id) const
{
    return id < statistics_.size() ? std::optional<PlaybackStats>(statistics_[id]) : std::nullopt;
}

float Mixer::next(Voice& voice)
{
    if (!voice.effect) return 0;
    const auto& sample = voice.effect->sample;
    const bool nativeLoop = voice.sampleLoop && sample.loopFlag;
    const bool looping = nativeLoop || voice.repeat;
    // First pass plays the complete payload; subsequent passes use loop bounds.
    const auto boundary = voice.inLoop && nativeLoop ? sample.loopEnd : sample.pcm.size();
    constexpr float ramp = 1.0f/32; // Short port-only anti-click ramp (6.4 ms).
    voice.envelope = std::clamp(voice.envelope + (voice.releasing ? -ramp : ramp),0.0f,1.0f);
    float gain = voice.envelope;
    if (!looping) gain = std::min(gain,float(boundary-voice.cursor)/32);
    const float result = sample.pcm[voice.cursor++] * gain * voice.effect->volume/64;
    ++statistics_[voice.id].sourceFrames;
    if (voice.releasing && voice.envelope == 0) {
        ++statistics_[voice.id].stopped;
        voice.effect = nullptr;
    }
    else if (voice.cursor == boundary) {
        if (looping) {
            ++statistics_[voice.id].wraps;
            voice.cursor = nativeLoop ? sample.loopBegin : 0;
            voice.inLoop = true;
        }
        else {
            ++statistics_[voice.id].completed;
            voice.effect = nullptr;
        }
    }
    return result;
}

void Mixer::render(float* output, std::size_t frames)
{
    for (std::size_t i = 0; i < frames; ++i) {
        float sample = 0;
        for (auto& voice : voices_) sample += next(voice);
        output[i] = std::clamp(sample * volume_ / 4,-1.0f,1.0f);
    }
}
} // namespace audio
