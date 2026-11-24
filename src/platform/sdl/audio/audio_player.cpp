#include <cstddef>
#include <filesystem>
#include <string>
#include <utility>
#include "platform/sdl/audio/audio_player.h"

#include <exception>

namespace audio {
bool Player::open(const std::filesystem::path& path, std::string& error, bool single)
{
    close();
    try {
        auto samples = mixerSamples(single ? Bank{{loadSample(path),64,0,0}} : loadBank(path));
        return player_.open(sampleRate, std::move(samples), error);
    }
    catch (const std::exception& e) {
        error = e.what();
        return false;
    }
}

bool Player::play(std::size_t id, bool repeat, Playback playback)
{
    return player_.play(id, playbackOptions(repeat, playback));
}
} // namespace audio
