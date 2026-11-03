#include "sprite_bank.h"
#include "spr_format.h"

#include <string>
#include <utility>

namespace sprites {
namespace {
Bank colorImages(std::vector<spr_format::Image> indexed,
                 const std::array<wall_textures::Rgb, 256>& palette)
{
    Bank bank;
    bank.images.reserve(indexed.size());
    for (auto& source : indexed) {
        Image image{{source.width, source.height, {}}, source.anchorX, source.anchorY,
                    std::move(source.indices)};
        image.texture.pixels.reserve(image.indices.size());
        for (auto index : image.indices) image.texture.pixels.push_back(palette[index]);
        bank.images.push_back(std::move(image));
    }
    return bank;
}
} // namespace

Bank decode(const std::vector<std::uint8_t>& bytes,
            const std::array<wall_textures::Rgb, 256>& palette)
{
    return colorImages(spr_format::decode(bytes), palette);
}

Bank load(const std::filesystem::path& directory, std::uint16_t objetBank,
          std::uint16_t palBank)
{
    return colorImages(spr_format::load(directory / ("OBJET" + std::to_string(objetBank) + ".SPR")),
        wall_textures::loadPalette(directory / ("PAL" + std::to_string(palBank) + ".PAL")));
}
} // namespace sprites
