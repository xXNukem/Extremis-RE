#pragma once
// VORTEX_DEBUG_PROFILER_V1
// SDL frame telemetry, collected only by instrumented builds.
#include <SDL3/SDL_render.h>

#include <array>
#include <cstddef>
#include <cstdint>

namespace platform::sdl::video_profiling {
enum Stage : std::size_t { Rays, Walls, Planes, Sprites, Hud, Composition, Present, Geometry, Queue, WorkerWorld, Count };

struct Snapshot {
    std::array<double, Count> ms{};
    double totalMs = 0.0;
    double otherMs = 0.0;
    double fps = 0.0;
};
class FrameProfiler {
public:
    const Snapshot& snapshot() const;
    const auto& frameSamplesNs() const { return frameNs; }

    void beginFrame();

    static void addSample(FrameProfiler* profiler, Stage stage, std::uint64_t durationNs);

    class Scope {
    public:
        Scope(FrameProfiler* profiler, Stage stage);
        ~Scope();
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;

    private:
        FrameProfiler* profiler_;
        Stage stage_;
        std::uint64_t startNs_;
    };

    bool timedPresent(SDL_Renderer* renderer);

    void reset();

    void endFrame();

private:
    Snapshot displayed{};
    std::array<std::uint64_t, Count> frameNs{};
    std::array<std::uint64_t, Count> accumulatedNs{};
    std::uint64_t frameStartNs=0;
    std::uint64_t intervalStartNs=0;
    std::uint64_t totalAccumulatedNs=0;
    unsigned samples=0;
};
} // namespace platform::sdl::video_profiling
