#include "extremis/render/first_person/presentation/first_person_view.h"
#include "extremis/ui/frame_composition.h"
#ifdef EXTREMIS_DEBUG_MENU
#include "platform/sdl/video/frame_profiler.h"
#endif

#include <SDL3/SDL.h>
#include <cmath>

bool FirstPersonRenderer::resizeFrame(int width, int height, bool withHud)
{
    const bool nativeView=withHud && video_.mode.nativeMax;
    if (width == width_ && height == height_ && withHud == withHud_ && nativeView==nativeComposed_) {
        return true;
    }
    auto texture=platform::sdl::createFrameTexture(renderer_,
        width, withHud ? (nativeView && height>hud::viewHeight
            ? int(std::lround(double(height)*hud::height/hud::viewHeight))
            : hud::height * (height / hud::viewHeight)) : height);
    if (!texture) return false;
    frameReady_=false; showingMap_=false;
    pixels_.resize(std::size_t(width) * height);
    composed_.resize(withHud ? hud::width * hud::height : 0);
    wideComposed_.resize(nativeView && height>hud::viewHeight
        ? std::size_t(width)*int(std::lround(double(height)*hud::height/hud::viewHeight))
        : (withHud && height>hud::viewHeight
        ? std::size_t(width)*(height+(hud::height-hud::viewHeight)*(height/hud::viewHeight))
        : (withHud && width>hud::width ? std::size_t(width)*hud::height : 0)));
    uploadTexture_ = std::move(texture);
    width_ = width;
    height_ = height;
    withHud_ = withHud;
    nativeComposed_ = nativeView;
    if (withHud) {
        int drawableWidth=0,drawableHeight=0;
        SDL_GetRenderOutputSize(renderer_,&drawableWidth,&drawableHeight);
        SDL_Log("VORTEX VIDEO: salida SDL=%dx%d; selector=%dx%d; mundo 3D=%dx%d; HUD DOS=320x200",
                drawableWidth,drawableHeight,
                video_.mode.requestedSize.width,video_.mode.requestedSize.height,width_,height_);
    }
    return true;
}

bool FirstPersonRenderer::presentFrame(bool withHud, int outputWidth, int outputHeight,
                                       const hud::Renderer* hudRenderer)
{
#ifdef EXTREMIS_DEBUG_MENU
    platform::sdl::video_profiling::FrameProfiler::Scope compositionTimer(profiler_,platform::sdl::video_profiling::Composition);
#endif

    const auto frame=extremis::composeFrame(pixels_,composed_,wideComposed_,
        width_,height_,withHud,nativeComposed_,hudRenderer);
    if (frame.error) return SDL_SetError("%s",frame.error);
    return platform::sdl::queueFrameTexture(renderer_,uploadTexture_.get(),frame.pixels,
                                          width_,outputWidth,outputHeight,frame.displayAspect);
}

bool FirstPersonRenderer::drawFrame(const std::vector<std::uint32_t>& pixels)
{
    if(pixels.size()!=64000) return SDL_SetError("Invalid presentation framebuffer");
    if(!resizeFrame(320,150,true)) return false;
    composed_=pixels;
    int width,height;
    return SDL_GetRenderOutputSize(renderer_,&width,&height) && presentFrame(true,width,height);
}
