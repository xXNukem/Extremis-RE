#pragma once

#include "texture_bank.h"

namespace sprites {

struct Image {
    wall_textures::Texture texture;
    int anchorX = 0, anchorY = 0;
    std::vector<std::uint8_t> indices; // Index 0 alone is transparent, regardless of RGB.
};

struct Bank {
    std::vector<Image> images;
};

Bank decode(const std::vector<std::uint8_t>& bytes,
            const std::array<wall_textures::Rgb, 256>& palette);
Bank load(const std::filesystem::path& directory, std::uint16_t objetBank,
          std::uint16_t palBank);

} // namespace sprites
