#pragma once

#include <SDL3/SDL.h>
#include <array>
#include <memory>
#include <vector>

namespace render_benchmark {
enum Stage { Clear, Background, Planes, Rays, Walls, Upload, Queue, Overlay, Present, Total, Count };
using Timings = std::array<double, Count>;
struct Target {
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window{nullptr, SDL_DestroyWindow};
    std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface{nullptr, SDL_DestroySurface};
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer{nullptr, SDL_DestroyRenderer};
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> texture{nullptr, SDL_DestroyTexture};
    int width = 0, height = 0;
};
struct Samples {
    std::vector<Timings> stages;
    std::vector<double> production;
};
Target createTarget(int width, int height, bool windowed);
double mean(const std::vector<double>& samples);
double percentile(std::vector<double> samples, double fraction);
} // namespace render_benchmark
