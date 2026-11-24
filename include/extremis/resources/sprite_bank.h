#pragma once

#include "extremis/resources/texture_bank.h"
#include "vortex/render/first_person/sprites/sprite_types.h"

namespace sprites {

using Image = vortex::sprites::Image;
using Bank = vortex::sprites::Bank;

Bank decode(const std::vector<std::uint8_t>& bytes,
            const std::array<wall_textures::Rgb, 256>& palette);
Bank load(const std::filesystem::path& file, const std::array<wall_textures::Rgb,256>& palette);
Bank load(const std::filesystem::path& directory, std::uint16_t objetBank,
          std::uint16_t palBank);

} // namespace sprites
