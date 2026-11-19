#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace vortex {
class FisheyeEffect {
public:
    struct Settings {
        double pixelAspect; // Physical pixel width divided by height, greater than zero.
        double strength;    // Radial distortion in [0, 1]. Zero preserves every pixel.
    };

    explicit FisheyeEffect(Settings settings) noexcept;

    // Rebuilds the lookup only when the dimensions change. Invalid inputs leave pixels intact.
    bool apply(std::vector<std::uint32_t>& pixels, int width, int height);

private:
    Settings settings_;
    int width_ = 0, height_ = 0;
    std::vector<std::size_t> lookup_;
    std::vector<std::uint32_t> destination_;
};
} // namespace vortex
