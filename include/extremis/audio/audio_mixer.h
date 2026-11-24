#pragma once

#include <vector>
#include "extremis/audio/playback_stats.h"
#include "extremis/resources/audio_data.h"
#include "vortex/audio/mixer.h"

namespace audio {
enum class Playback { Native, Once };

inline constexpr int sampleRate = 5000;
inline constexpr vortex::audio::MixerConfig mixerConfig{4, 32, 0.6f};

std::vector<vortex::audio::Sample> mixerSamples(Bank bank);
vortex::audio::PlayOptions playbackOptions(bool repeat, Playback playback);
} // namespace audio
