#pragma once

#include <cstdint>
#include <memory>

struct SDL_Renderer;
struct SDL_Texture;

namespace platform::sdl {
struct FrameTextureDeleter {
    void operator()(SDL_Texture* texture) const;
};

using FrameTexture = std::unique_ptr<SDL_Texture, FrameTextureDeleter>;

FrameTexture createFrameTexture(SDL_Renderer* renderer, int width, int height);
bool queueFrameTexture(SDL_Renderer* renderer, SDL_Texture* texture, const std::uint32_t* pixels,
                       int width, int outputWidth, int outputHeight, float displayAspect);
}
