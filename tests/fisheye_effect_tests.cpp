#include "fisheye_effect.h"

#include <cassert>
#include <cstdint>
#include <vector>

int main()
{
    FisheyeEffect effect;
    // Native DOS view and progressively wider aspect ratios, including 21:9.
    for (const int width : {320, 384, 427, 560, 960}) {
        constexpr int height = 150;
        std::vector<std::uint32_t> source(std::size_t(width) * height);
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
                source[std::size_t(y) * width + x] = std::uint32_t(x) | (std::uint32_t(y) << 16);
        auto pixels = source;
        assert(effect.apply(pixels, width, height));
        assert(pixels.size() == source.size());
        // No black borders or missing corners, even on an ultrawide output.
        assert(pixels.front() == source.front());
        assert(pixels[width - 1] == source[width - 1]);
        assert(pixels[(height - 1) * width] == source[(height - 1) * width]);
        assert(pixels.back() == source.back());
        // A visible lens bend: a vertical feature halfway to the left edge
        // bows farther left at the screen centre, and similarly on the right.
        const int y = height / 2;
        const int left = width / 4, right = width * 3 / 4;
        assert((pixels[std::size_t(y) * width + left] & 0xffff) > unsigned(left));
        assert((pixels[std::size_t(y) * width + right] & 0xffff) < unsigned(right));
        // Stable output over multiple frames; the lookup is reused.
        auto repeat = source;
        assert(effect.apply(repeat, width, height));
        assert(repeat == pixels);
    }
    std::vector<std::uint32_t> invalid(12);
    assert(!effect.apply(invalid, 1, 12));
    assert(!effect.apply(invalid, 4, 4));
}
