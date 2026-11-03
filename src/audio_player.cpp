#include "audio_player.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <exception>

namespace audio {
Player::Player() : mixer_(bank_) {}
Player::~Player() { close(); }

bool Player::open(const std::filesystem::path& path, std::string& error, bool single)
{
    close();
    error.clear();
    try { bank_ = single ? Bank{{loadSample(path),64,0,0}} : loadBank(path); }
    catch (const std::exception& e) { error = e.what(); return false; }
    mixer_.resetStatistics();
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        error = SDL_GetError();
        close();
        return false;
    }
    initialized_ = true;
    const SDL_AudioSpec spec{SDL_AUDIO_F32,1,5000};
    stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,feed,this);
    if (!stream_ || !SDL_ResumeAudioStreamDevice(stream_)) {
        error = SDL_GetError();
        close();
        return false;
    }
    return true;
}

void Player::close()
{
    if (stream_) {
        // Setting a null callback waits for any running callback to finish.
        SDL_SetAudioStreamGetCallback(stream_,nullptr,nullptr);
        SDL_DestroyAudioStream(stream_);
        stream_ = nullptr;
    }
    mixer_.clear();
    // Voices reference effects in this bank: release it only after the callback
    // has stopped and all voices have been cleared, including failed opens.
    Bank{}.swap(bank_);
    mixer_.resetStatistics();
    if (initialized_) SDL_QuitSubSystem(SDL_INIT_AUDIO);
    initialized_ = false;
}

void SDLCALL Player::feed(void* userdata, SDL_AudioStream* stream, int additional, int)
{
    auto& player = *static_cast<Player*>(userdata);
    std::array<float,512> buffer{};
    // SDL holds the stream lock while invoking the callback. No allocations,
    // filesystem access, or game/render dependencies in the producer.
    while (additional > 0) {
        const int frames = std::min<int>(buffer.size(),(additional-1)/int(sizeof(float))+1);
        player.mixer_.render(buffer.data(),frames);
        if (!SDL_PutAudioStreamData(stream,buffer.data(),frames * int(sizeof(float)))) return;
        additional -= frames * int(sizeof(float));
    }
}

bool Player::play(std::size_t id, bool repeat, Playback playback)
{
    if (!stream_ || !SDL_LockAudioStream(stream_)) return false;
    const bool result = mixer_.play(id,repeat,playback);
    SDL_UnlockAudioStream(stream_);
    return result;
}
void Player::stop(std::size_t id)
{
    if (!stream_ || !SDL_LockAudioStream(stream_)) return;
    mixer_.stop(id);
    SDL_UnlockAudioStream(stream_);
}
void Player::setVolume(float volume)
{
    if (!stream_ || !SDL_LockAudioStream(stream_)) return;
    mixer_.setVolume(volume);
    SDL_UnlockAudioStream(stream_);
}
std::size_t Player::effectCount() const { return bank_.size(); }
std::size_t Player::activeVoices()
{
    if (!stream_ || !SDL_LockAudioStream(stream_)) return 0;
    const auto count = mixer_.active();
    SDL_UnlockAudioStream(stream_);
    return count;
}
std::optional<PlaybackStats> Player::statistics(std::size_t id)
{
    if (!stream_ || !SDL_LockAudioStream(stream_)) return std::nullopt;
    const auto stats = mixer_.statistics(id);
    SDL_UnlockAudioStream(stream_);
    return stats;
}

bool Player::playing(std::size_t id)
{
    if (!stream_ || !SDL_LockAudioStream(stream_)) return false;
    const bool active = mixer_.playing(id);
    SDL_UnlockAudioStream(stream_);
    return active;
}
} // namespace audio
