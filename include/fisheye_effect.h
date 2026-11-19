#pragma once

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <vector>

// Mild barrel distortion of the 3D world only. The HUD is composed afterwards.
// The lookup is rebuilt only when the internal view dimensions change.
class FisheyeEffect {
public:
    bool apply(std::vector<std::uint32_t>& pixels, int width, int height);

private:
    int width_ = 0, height_ = 0;
    std::vector<std::size_t> lookup_;
    std::vector<std::uint32_t> destination_;
};

inline bool FisheyeEffect::apply(std::vector<std::uint32_t>& pixels, int width, int height)
{
    if (width < 2 || height < 2 || pixels.size() != std::size_t(width) * height)
        return false;

    if (width_ != width || height_ != height || lookup_.size() != pixels.size()) {
        // VGA's 320x200 image displays as 4:3: an X pixel occupies 5/6 of
        // a Y pixel in physical screen space. This avoids oval distortion.
        constexpr double pixelAspect = 5.0 / 6.0;
        constexpr double strength = 0.20;
        const double cx = (width - 1) * 0.5, cy = (height - 1) * 0.5;
        const double extent2 = cx * cx * pixelAspect * pixelAspect + cy * cy;
        if (extent2 <= 0.0) return false;
        lookup_.resize(pixels.size());
        for (int y = 0; y < height; ++y) {
            const double dy = y - cy;
            for (int x = 0; x < width; ++x) {
                const double dx = x - cx;
                const double radius2 = (dx * dx * pixelAspect * pixelAspect + dy * dy) / extent2;
                // Radial barrel warp. At all corners scale=1; no black borders,
                // repeated edge columns or out-of-bounds sampling.
                const double scale = 1.0 - strength * (1.0 - std::min(radius2, 1.0));
                const int sampleX = std::clamp(int(std::lround(cx + dx * scale)), 0, width - 1);
                const int sampleY = std::clamp(int(std::lround(cy + dy * scale)), 0, height - 1);
                lookup_[std::size_t(y) * width + x] = std::size_t(sampleY) * width + sampleX;
            }
        }
        width_ = width;
        height_ = height;
    }

    const auto count = pixels.size();
    if (destination_.size() != count) {
        if (destination_.capacity() < count)
            destination_ = std::vector<std::uint32_t>(count);
        else
            destination_.resize(count);
    }
    const auto* source = pixels.data();
    const auto* lookup = lookup_.data();
    auto* destination = destination_.data();
    for (std::size_t i = 0; i < count; ++i)
        destination[i] = source[lookup[i]];
    // Publish the warped frame and retain the old storage for the next draw.
    pixels.swap(destination_);
    return true;
}
