#include "sprite_scene.h"

#include <array>
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace sprites {
namespace {
// EXE file 0x5d23: level-1 height table selected by 0x7d00..0x7daf.
constexpr std::array<double, 22> heights{
    100,80,50,50,255,255,150,80,60,165,255,255,50,35,10,255,255,255,150,255,150,150
};

void copyPatch(Image& target, const Image& source, int x, int y)
{
    const auto& src = source.texture;
    auto& dst = target.texture;
    if (x < 0 || y < 0 || x + src.width > dst.width || y + src.height > dst.height ||
        src.width <= 0 || src.height <= 0 || source.indices.size() != src.pixels.size() ||
        target.indices.size() != dst.pixels.size())
        throw std::runtime_error("Invalid original OBJET composition bounds");
    for (int row = 0; row < src.height; ++row) {
        const auto from = std::size_t(row) * src.width;
        const auto to = std::size_t(y + row) * dst.width + x;
        std::copy_n(source.indices.begin() + from, src.width, target.indices.begin() + to);
        std::copy_n(src.pixels.begin() + from, src.width, dst.pixels.begin() + to);
    }
}

void composeInitialImages(Bank& bank)
{
    // EXE 0x14d4c..0x14e10 at global tick 0; API sprite indices there are one-based.
    // These are opaque memory copies, including index 0. Freeze the verified
    // initial composition; entity animation is deliberately not advanced here.
    if (bank.images.size() != 44) throw std::runtime_error("Unvalidated OBJET1 image count");
    for (const auto& patch : std::array<std::array<int, 4>, 6>{{
            {15,22,3,27}, {15,23,3,51}, {16,30,2,21},
            {17,33,15,65}, {17,38,15,88}, {17,40,15,104}}}) {
        copyPatch(bank.images[patch[0]], bank.images[patch[1]], patch[2], patch[3]);
    }
}
}

std::optional<std::size_t> imageIndex(std::uint16_t type, std::size_t count)
{
    const auto family = type & 0xf000;
    const std::size_t index = type & 0xff;
    if ((family != 0x4000 && family != 0x5000) || index >= count) return std::nullopt;
    return index;
}

Scene makeScene(const game_lab::Level& level, Bank bank)
{
    Scene scene{std::move(bank), {}, 0};
    for (const auto& sector : level.sectors) {
        for (const auto& entity : sector.entitiesA) {
            const auto image = imageIndex(entity.type, scene.bank.images.size());
            if (level.objetBank != 1 || !image || *image >= heights.size() ||
                entity.vertex >= sector.vertices.size()) {
                ++scene.skipped;
                continue;
            }
            const auto& vertex = sector.vertices[entity.vertex];
            scene.objects.push_back({double(sector.gx) + vertex.x,
                double(sector.gy) + vertex.y, double(entity.param1), heights[*image],
                *image, entity.type, entity.param2});
        }
    }
    return scene;
}

Scene loadScene(const game_lab::Level& level, const std::filesystem::path& directory)
{
    auto bank = load(directory, level.objetBank, level.palBank);
    if (level.objetBank == 1) composeInitialImages(bank);
    return makeScene(level, std::move(bank));
}
} // namespace sprites
