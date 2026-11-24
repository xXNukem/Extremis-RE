#pragma once

struct SDL_AudioStream;

namespace startup_credits {
class CreditsAudio {
public:
    CreditsAudio() = default;
    CreditsAudio(const CreditsAudio&) = delete;
    CreditsAudio& operator=(const CreditsAudio&) = delete;
    ~CreditsAudio();

    void start(float gain);
    void setFocused(bool focused);

private:
    SDL_AudioStream* stream=nullptr;
    bool initialized=false;
};
}
