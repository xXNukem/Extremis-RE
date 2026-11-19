#pragma once

#include <SDL3/SDL_render.h>

namespace platform::sdl {

class RenderState {
public:
    explicit RenderState(SDL_Renderer* renderer);

    RenderState(const RenderState&) = delete;
    RenderState& operator=(const RenderState&) = delete;

    ~RenderState();

private:
    SDL_Renderer* renderer_;
    int width_ = 0;
    int height_ = 0;
    float scaleX_ = 1;
    float scaleY_ = 1;
    SDL_RendererLogicalPresentation mode_ = SDL_LOGICAL_PRESENTATION_DISABLED;
    Uint8 red_ = 0;
    Uint8 green_ = 0;
    Uint8 blue_ = 0;
    Uint8 alpha_ = 255;
};

} // namespace platform::sdl
