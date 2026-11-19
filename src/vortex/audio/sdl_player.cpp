#include "vortex/audio/sdl_player.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <utility>

namespace vortex::audio {
Player::Player(MixerConfig config) : mixer_(config) {}
Player::~Player() { close(); }

bool Player::open(int rate, std::vector<Sample> samples, std::string& error)
{
    close();
    error.clear();
    if (rate <= 0) {
        error = "Audio source rate must be positive";
        return false;
    }
    mixer_.setSamples(std::move(samples));
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        error = SDL_GetError();
        close();
        return false;
    }
    initialized_ = true;
    const SDL_AudioSpec spec{SDL_AUDIO_F32,1,rate};
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
    // Release the samples only after the callback has stopped.
    mixer_.setSamples({});
    if (initialized_) SDL_QuitSubSystem(SDL_INIT_AUDIO);
    initialized_ = false;
}

void SDLCALL Player::feed(void* userdata, SDL_AudioStream* stream, int additional, int)
{
    auto& player = *static_cast<Player*>(userdata);
    std::array<float,512> buffer{};
    // SDL holds the stream lock while invoking the callback. The producer
    // uses a fixed scratch buffer and submits it through the SDL stream.
    while (additional > 0) {
        const int frames = std::min<int>(buffer.size(),(additional-1)/int(sizeof(float))+1);
        player.mixer_.render(buffer.data(),frames);
        if (!SDL_PutAudioStreamData(stream,buffer.data(),frames * int(sizeof(float)))) return;
        additional -= frames * int(sizeof(float));
    }
}

bool Player::play(std::size_t id, PlayOptions options)
{
    if (!stream_ || !SDL_LockAudioStream(stream_)) return false;
    const bool result = mixer_.play(id,options);
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
void Player::pause(bool paused)
{
    if (stream_) {
        if (paused) SDL_PauseAudioStreamDevice(stream_);
        else SDL_ResumeAudioStreamDevice(stream_);
    }
}
std::size_t Player::sampleCount() const { return mixer_.sampleCount(); }
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
} // namespace vortex::audio
