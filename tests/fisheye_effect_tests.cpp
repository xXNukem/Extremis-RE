#include <cstddef>
#include "vortex/render/first_person/postprocess/fisheye_effect.h"

#include <cstdlib>
#include <iostream>
#include <cstdint>
#include <vector>

namespace {
void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    vortex::FisheyeEffect effect{{5.0 / 6.0, 0.20}};
    vortex::FisheyeEffect squarePixels{{1.0, 0.20}};
    vortex::FisheyeEffect identity{{5.0 / 6.0, 0.0}};
    // Native DOS view and progressively wider aspect ratios, including 21:9.
    for (const int width : {320, 384, 427, 560, 960}) {
        constexpr int height = 150;
        std::vector<std::uint32_t> source(std::size_t(width) * height);
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
                source[std::size_t(y) * width + x] = std::uint32_t(x) | (std::uint32_t(y) << 16);
        auto pixels = source;
        require(effect.apply(pixels, width, height), "Fisheye rejected valid dimensions");
        require(pixels.size() == source.size(), "Fisheye changed the pixel count");
        // No black borders or missing corners, even on an ultrawide output.
        require(pixels.front() == source.front(), "Fisheye changed the top-left corner");
        require(pixels[width - 1] == source[width - 1], "Fisheye changed the top-right corner");
        require(pixels[(height - 1) * width] == source[(height - 1) * width], "Fisheye changed the bottom-left corner");
        require(pixels.back() == source.back(), "Fisheye changed the bottom-right corner");
        // A visible lens bend: a vertical feature halfway to the left edge
        // bows farther left at the screen centre, and similarly on the right.
        const int y = height / 2;
        const int left = width / 4, right = width * 3 / 4;
        require((pixels[std::size_t(y) * width + left] & 0xffff) > unsigned(left), "Fisheye did not bend the left feature");
        require((pixels[std::size_t(y) * width + right] & 0xffff) < unsigned(right), "Fisheye did not bend the right feature");
        auto square = source;
        require(squarePixels.apply(square, width, height) && square != pixels,
                "Pixel aspect did not affect the warp");
        auto undistorted = source;
        require(identity.apply(undistorted, width, height) && undistorted == source,
                "Zero strength changed the framebuffer");
        // Stable output over multiple frames; the lookup is reused.
        auto repeat = source;
        require(effect.apply(repeat, width, height), "Fisheye rejected valid dimensions on reuse");
        require(repeat == pixels, "Fisheye output changed after lookup reuse");
    }
    std::vector<std::uint32_t> invalid(12);
    require(!effect.apply(invalid, 1, 12), "Fisheye accepted a one-pixel width");
    require(!effect.apply(invalid, 4, 4), "Fisheye accepted mismatched dimensions");
}
