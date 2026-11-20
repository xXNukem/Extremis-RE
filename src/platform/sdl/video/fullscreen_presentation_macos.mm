#include "platform/sdl/video/render_state.h"

#import <QuartzCore/CAMetalLayer.h>

namespace platform::sdl {

void configureMetalFullscreenPresentation(SDL_Renderer* renderer)
{
    if (@available(macOS 10.13.2, *)) {
        auto* layer = (__bridge CAMetalLayer*)SDL_GetRenderMetalLayer(renderer);
        if (layer && layer.maximumDrawableCount == 3) {
            // In a fullscreen Space, three drawables can pace acquisition to
            // refresh with VSync off. Set the pool before the first present.
            layer.maximumDrawableCount = 2;
        }
    }
}

} // namespace platform::sdl
