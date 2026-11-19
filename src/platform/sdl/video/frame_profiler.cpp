#include "platform/sdl/video/frame_profiler.h"

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_timer.h>

namespace platform::sdl::video_profiling {

const Snapshot& FrameProfiler::snapshot() const
{
    return displayed;
}

void FrameProfiler::beginFrame()
{
    frameNs.fill(0);
    frameStartNs=SDL_GetTicksNS();
}

void FrameProfiler::addSample(FrameProfiler* profiler, Stage stage, std::uint64_t durationNs)
{
    if (profiler) profiler->frameNs[stage]+=durationNs;
}

FrameProfiler::Scope::Scope(FrameProfiler* profiler, Stage stage)
    : profiler_(profiler), stage_(stage), startNs_(SDL_GetTicksNS())
{}

FrameProfiler::Scope::~Scope()
{
    addSample(profiler_,stage_,SDL_GetTicksNS()-startNs_);
}

bool FrameProfiler::timedPresent(SDL_Renderer* renderer)
{
    const auto start=SDL_GetTicksNS();
    const bool result=SDL_RenderPresent(renderer);
    addSample(this,Present,SDL_GetTicksNS()-start);
    return result;
}

void FrameProfiler::reset()
{
    frameNs.fill(0);
    accumulatedNs.fill(0);
    frameStartNs=0;
    intervalStartNs=0;
    totalAccumulatedNs=0;
    samples=0;
    displayed=Snapshot{};
}

void FrameProfiler::endFrame()
{
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

} // namespace platform::sdl::video_profiling
