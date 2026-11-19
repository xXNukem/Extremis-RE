#include "platform/sdl/video/render_state.h"

#include <stdexcept>

namespace platform::sdl {

RenderState::RenderState(SDL_Renderer* renderer) : renderer_(renderer)
{
    if (!SDL_GetRenderLogicalPresentation(renderer_, &width_, &height_, &mode_) ||
        !SDL_GetRenderScale(renderer_, &scaleX_, &scaleY_) ||
        !SDL_GetRenderDrawColor(renderer_, &red_, &green_, &blue_, &alpha_)) {
        throw std::runtime_error(SDL_GetError());
    }
}

RenderState::~RenderState()
{
    SDL_SetRenderLogicalPresentation(renderer_, width_, height_, mode_);
    SDL_SetRenderScale(renderer_, scaleX_, scaleY_);
    SDL_SetRenderDrawColor(renderer_, red_, green_, blue_, alpha_);
}

} // namespace platform::sdl
