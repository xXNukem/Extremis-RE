#include "audio_player.h"
#include <SDL3/SDL.h>
#include <cmath>
#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[])
{
    if (argc < 2 || argc > 4) {
        std::cerr << "Usage: audio_probe sample.DSS\n"
                     "       audio_probe BANK.DSS original-id [seconds, default 3]\n";
        return 1;
    }
    try {
        const bool single = argc == 2;
        const int id = single ? 0 : std::stoi(argv[2]);
        const double seconds = argc == 4 ? std::stod(argv[3]) : 3;
        if (id < 0 || !std::isfinite(seconds) || seconds <= 0 || seconds > 120)
            throw std::runtime_error("Invalid ID or diagnostic duration (0..120 seconds)");
        audio::Player player;
        std::string error;
        if (!player.open(argv[1],error,single)) throw std::runtime_error(error);
        if (!player.play(id)) throw std::runtime_error("Invalid sound ID or unavailable voice");
        std::cout << "Playing original " << argv[1] << " ID=" << id << " (5000 Hz mono)\n";
        SDL_Delay(Uint32(seconds*1000));
        player.stop(id);
        SDL_Delay(30); // Let the release ramp reach silence before destroying the stream.
        if (const auto stats = player.statistics(id)) {
            std::cout << "decodedFrames=" << stats->decodedFrames << " requests=" << stats->requests
                      << " started=" << stats->started << " sourceFrames=" << stats->sourceFrames
                      << " wraps=" << stats->wraps << " completed=" << stats->completed
                      << " stopped=" << stats->stopped << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
