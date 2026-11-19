#pragma once

#include <cstdint>

struct SDL_Renderer;
namespace platform::sdl::video_profiling { struct Snapshot; }

namespace app::diagnostics {
class GameplayFps {
public:
    void recordPresent(std::uint64_t now);
    void resetInterval();
    double value() const;

private:
    std::uint64_t previousPresentNs_ = 0;
    std::uint64_t accumulatedNs_ = 0;
    unsigned frameCount_ = 0;
    double fps_ = 0.0;
};


bool drawGameplayFps(SDL_Renderer* renderer, double gameplayFps,
                     const platform::sdl::video_profiling::Snapshot& snapshot);
}
