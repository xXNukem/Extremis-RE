#include "vortex/audio/sdl_player.h"

#include <SDL3/SDL.h>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}
}

int main()
{
    using namespace vortex::audio;
    try {
        check(SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy"), "Cannot select SDL dummy audio driver");
        Player player({2, 8});
        const std::vector<Sample> samples{{std::vector<float>(64, 0.25f), 1.0f, Loop{2, 62, 64}}};
        std::string error;
        const auto open = [&](Player& target) {
            if (!target.open(16000, samples, error)) throw std::runtime_error(error);
        };
        open(player);
        player.pause(true);
        check(player.play(0), "Could not queue PCM while paused");
        SDL_Delay(80);
        auto stats = player.statistics(0);
        check(stats && stats->started == 1 && stats->sourceFrames == 0, "Paused callback consumed PCM");

        player.pause(false);
        const auto waitFor = [&](auto predicate, const char* message) {
            const auto deadline = SDL_GetTicks() + 1500;
            while (!predicate() && SDL_GetTicks() < deadline) SDL_Delay(5);
            check(predicate(), message);
        };
        waitFor([&] { return player.statistics(0)->wraps != 0; }, "Resumed callback did not consume PCM");
        player.pause(true);
        const auto pausedFrames = player.statistics(0)->sourceFrames;
        SDL_Delay(80);
        check(player.statistics(0)->sourceFrames == pausedFrames, "Second pause did not freeze PCM");
        player.stop(0);
        player.pause(false);
        waitFor([&] { return player.statistics(0)->stopped == 1; }, "Callback did not finish release");

        Player other({1, 0});
        open(other);
        check(player.play(0), "Could not start loop before close");
        player.close();
        player.close();
        check(player.sampleCount() == 0 && player.activeVoices() == 0 &&
              !player.statistics(0) && !player.play(0), "Close retained playback state");
        check(other.play(0), "Close invalidated the second SDL owner");
        open(player);
        check(player.play(0), "Could not start loop before destruction");
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
