#include "gameplay_fps.h"

#include "platform/sdl/video/frame_profiler.h"
#include <SDL3/SDL_render.h>

namespace app::diagnostics {
namespace profiling = platform::sdl::video_profiling;

void GameplayFps::recordPresent(std::uint64_t now)
{
    if (previousPresentNs_ && now > previousPresentNs_) {
        accumulatedNs_ += now - previousPresentNs_;
        ++frameCount_;
        if (accumulatedNs_ >= 500000000ULL) {
            fps_ = double(frameCount_) * 1000000000.0 / double(accumulatedNs_);
            accumulatedNs_ = 0;
            frameCount_ = 0;
        }
    }
    previousPresentNs_ = now;
}

void GameplayFps::resetInterval()
{
    previousPresentNs_ = 0;
    accumulatedNs_ = 0;
    frameCount_ = 0;
}

bool drawGameplayFps(SDL_Renderer* renderer, double gameplayFps, const profiling::Snapshot& perf)
{
    // This overlay is drawn outside the DOS framebuffer.
    int outputWidth=0, outputHeight=0;
    if (!SDL_GetRenderOutputSize(renderer,&outputWidth,&outputHeight)) return false;
    float previousScaleX=1.f, previousScaleY=1.f;
    if (!SDL_GetRenderScale(renderer,&previousScaleX,&previousScaleY)) return false;
    const float scale=outputHeight>=1800?3.f:outputHeight>=900?2.f:1.f;
    const SDL_FRect plate{8.f,8.f,200.f,171.f};
    const bool drawn=SDL_SetRenderScale(renderer,scale,scale) &&
        SDL_SetRenderDrawColor(renderer,0,0,0,255) &&
        SDL_RenderFillRect(renderer,&plate) &&
        SDL_SetRenderDrawColor(renderer,255,230,70,255) &&
        SDL_RenderDebugTextFormat(renderer,14.f,16.f,"FPS: %.1f",gameplayFps) &&
        SDL_RenderDebugTextFormat(renderer,14.f,32.f,"FRAME: %.1f ms",perf.totalMs) &&
        SDL_RenderDebugTextFormat(renderer,14.f,48.f,"RAY: %.1f ms",perf.ms[profiling::Rays]) &&
        SDL_RenderDebugTextFormat(renderer,14.f,64.f,"WALL: %.1f ms",perf.ms[profiling::Walls]) &&
        SDL_RenderDebugTextFormat(renderer,14.f,80.f,"PLANES: %.1f ms",perf.ms[profiling::Planes]) &&
        SDL_RenderDebugTextFormat(renderer,14.f,96.f,"SPRITES: %.1f ms",perf.ms[profiling::Sprites]) &&
        SDL_RenderDebugTextFormat(renderer,14.f,112.f,"HUD: %.1f ms",perf.ms[profiling::Hud]) &&
        SDL_RenderDebugTextFormat(renderer,14.f,128.f,"COMP/SDL: %.1f ms",perf.ms[profiling::Composition]) &&
        SDL_RenderDebugTextFormat(renderer,14.f,144.f,"PRESENT: %.1f ms",perf.ms[profiling::Present]) &&
        SDL_RenderDebugTextFormat(renderer,14.f,160.f,"OTHER: %.1f ms",perf.otherMs);
    const bool restored=SDL_SetRenderScale(renderer,previousScaleX,previousScaleY);
    return drawn && restored;
}
}

namespace app::diagnostics {

double GameplayFps::value() const
{
    return fps_;
}

} // namespace app::diagnostics
