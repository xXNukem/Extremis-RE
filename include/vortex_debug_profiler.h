#pragma once
// VORTEX_DEBUG_PROFILER_V1
// Header-only telemetry. Included ONLY from EXTREMIS_DEBUG_MENU branches.
#include <SDL3/SDL.h>
#include <array>
#include <algorithm>
#include <cstdint>

namespace vortex_debug_profiler {
enum Stage : std::size_t { Rays, Walls, Planes, Sprites, Hud, Composition, Present, Count };

struct Snapshot {
    std::array<double, Count> ms{};
    double totalMs = 0.0;
    double otherMs = 0.0;
    double fps = 0.0;
};
inline Snapshot displayed{};
inline std::array<std::uint64_t, Count> frameNs{};
inline std::array<std::uint64_t, Count> accumulatedNs{};
inline std::uint64_t frameStartNs=0;
inline std::uint64_t intervalStartNs=0;
inline std::uint64_t totalAccumulatedNs=0;
inline unsigned samples=0;

inline void beginFrame() {
    frameNs.fill(0);
    frameStartNs=SDL_GetTicksNS();
}
inline void add(Stage stage, std::uint64_t durationNs) {
    frameNs[stage]+=durationNs;
}
struct Scope {
    Stage stage;
    std::uint64_t startNs;
    explicit Scope(Stage stage):stage(stage),startNs(SDL_GetTicksNS()){}
    ~Scope() {add(stage,SDL_GetTicksNS()-startNs);}
};
inline bool timedPresent(SDL_Renderer* renderer) {
    const auto start=SDL_GetTicksNS();
    const bool result=SDL_RenderPresent(renderer);
    add(Present,SDL_GetTicksNS()-start);
    return result;
}
inline void reset() {
    frameNs.fill(0);
    accumulatedNs.fill(0);
    frameStartNs=0;
    intervalStartNs=0;
    totalAccumulatedNs=0;
    samples=0;
    displayed=Snapshot{};
}
inline void endFrame() {
    const auto now=SDL_GetTicksNS();
    if (!frameStartNs || now<frameStartNs) return;
    if (!intervalStartNs) intervalStartNs=frameStartNs;
    totalAccumulatedNs+=now-frameStartNs;
    for(std::size_t i=0;i<Count;++i) accumulatedNs[i]+=frameNs[i];
    ++samples;
    const auto interval=now-intervalStartNs;
    if (interval<1000000000ULL) return;

    // Per-presented-frame averages over the same stable display interval.
    const double invFrames=1e-6/double(samples);
    std::uint64_t measuredNs=0;
    for(std::size_t i=0;i<Count;++i) {
        displayed.ms[i]=double(accumulatedNs[i])*invFrames;
        measuredNs+=accumulatedNs[i];
    }
    displayed.totalMs=double(totalAccumulatedNs)*invFrames;
    displayed.otherMs=double(totalAccumulatedNs>measuredNs?totalAccumulatedNs-measuredNs:0)*invFrames;
    displayed.fps=double(samples)*1e9/double(interval);
    SDL_Log("VORTEX PERF DEBUG: fps=%.1f frame=%.2fms ray=%.2fms wall=%.2fms planes=%.2fms sprites=%.2fms hud=%.2fms compose+SDL=%.2fms present=%.2fms other=%.2fms",
        displayed.fps,displayed.totalMs,displayed.ms[Rays],displayed.ms[Walls],
        displayed.ms[Planes],displayed.ms[Sprites],displayed.ms[Hud],
        displayed.ms[Composition],displayed.ms[Present],displayed.otherMs);
    accumulatedNs.fill(0);
    intervalStartNs=now;
    totalAccumulatedNs=0;
    samples=0;
}
} // namespace vortex_debug_profiler
