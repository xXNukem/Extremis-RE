#include <cstdint>
#include "platform/sdl/video/frame_texture.h"

#include <SDL3/SDL_render.h>
#include <algorithm>

namespace platform::sdl {
void FrameTextureDeleter::operator()(SDL_Texture* texture) const
{
    SDL_DestroyTexture(texture);
}

FrameTexture createFrameTexture(SDL_Renderer* renderer, int width, int height)
{
    // RGBA8888 specifies channel bit positions, independent of host byte order.
    FrameTexture texture(SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                           SDL_TEXTUREACCESS_STREAMING, width, height));
    if (!texture || !SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_NONE) ||
        !SDL_SetTextureScaleMode(texture.get(), SDL_SCALEMODE_NEAREST)) {
        return {};
    }
    return texture;
}

bool queueFrameTexture(SDL_Renderer* renderer, SDL_Texture* texture, const std::uint32_t* pixels,
                       int width, int outputWidth, int outputHeight, float displayAspect)
{
    if (!SDL_UpdateTexture(texture, nullptr, pixels, width * int(sizeof(std::uint32_t))))
        return false;
    if (!displayAspect) return SDL_RenderTexture(renderer, texture, nullptr, nullptr);
    const float w=std::min(float(outputWidth),float(outputHeight)*displayAspect);
    const float h=w/displayAspect;
    const SDL_FRect destination{(outputWidth-w)*.5f,(outputHeight-h)*.5f,w,h};
    return SDL_RenderTexture(renderer, texture, nullptr, &destination);
}
}
