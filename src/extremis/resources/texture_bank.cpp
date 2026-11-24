#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>
#include "extremis/resources/texture_bank.h"
#include "extremis/resources/spr_format.h"
#include "extremis/resources/vga_palette.h"

#include <array>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace wall_textures {
namespace {
constexpr int width = 60;
constexpr int height = 74;
constexpr std::size_t imageBytes = width * height;
constexpr std::size_t bankBytes = 239760;

std::vector<std::uint8_t> readExactFile(const std::filesystem::path& path,
                                      std::size_t expectedSize)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Cannot open " + path.string());
    }
    if (file.tellg() != static_cast<std::streamoff>(expectedSize)) {
        throw std::runtime_error("Unsupported size for " + path.string() +
                                 "; expected " + std::to_string(expectedSize) + " bytes");
    }
    std::vector<std::uint8_t> bytes(expectedSize);
    file.seekg(0);
    if (!file.read(reinterpret_cast<char*>(bytes.data()), bytes.size())) {
        throw std::runtime_error("Cannot read " + path.string());
    }
    return bytes;
}

} // namespace

std::array<Rgb, 256> loadPalette(const std::filesystem::path& path)
{
    const auto bytes = readExactFile(path, 768);
    const auto decoded = vga_palette::decode(bytes.data(), "Invalid 6-bit palette component in " + path.string());
    std::array<Rgb, 256> palette{};
    for (std::size_t i = 0; i < palette.size(); ++i) {
        const auto& rgb = decoded[i];
        palette[i] = {rgb[0], rgb[1], rgb[2]};
    }
    return palette;
}
Bank loadWallOverlays(const std::filesystem::path& dataDirectory, std::uint16_t palBank)
{
    const auto bytes = readExactFile(dataDirectory / "ADALLE.SPR", 9972);
    const auto palette = loadPalette(dataDirectory / ("PAL" + std::to_string(palBank) + ".PAL"));
    const auto images = spr_format::decode(bytes);
    if (images.size() != 20) {
        throw std::runtime_error("Unsupported ADALLE offset table");
    }
    Bank bank;
    for (std::size_t i = 0; i < 20; ++i) {
        const int w = i < 4 ? 38 : i < 12 ? 24 : i < 18 ? 15 : 37;
        const int h = i < 4 ? 31 : i < 18 ? 17 : 3;
        if (images[i].width != w || images[i].height != h) {
            throw std::runtime_error("Unsupported ADALLE image dimensions");
        }
        Texture image{w, h, {}};
        image.pixels.reserve(w * h);
        for (auto index : images[i].indices) image.pixels.push_back(palette[index]);
        bank.textures.push_back(std::move(image));
    }
    return bank;
}

PlaneBank loadPlanes(const std::filesystem::path& dataDirectory, std::uint16_t solBank,
                     std::uint16_t palBank)
{
    const auto palette = loadPalette(dataDirectory / ("PAL" + std::to_string(palBank) + ".PAL"));
    const auto bytes = readExactFile(dataDirectory / ("SOL" + std::to_string(solBank) + ".DAS"), 32768);
    PlaneBank bank{{128, 128, {}}, {128, 128, {}}};
    bank.floor.pixels.reserve(16384);
    bank.ceiling.pixels.reserve(16384);
    for (int y = 0; y < 128; ++y) {
        for (int x = 0; x < 128; ++x) {
            bank.floor.pixels.push_back(palette[bytes[y * 256 + x]]);
            bank.ceiling.pixels.push_back(palette[bytes[y * 256 + 128 + x]]);
        }
    }
    return bank;
}

Bank load(const std::filesystem::path& dataDirectory, std::uint16_t dalleBank,
          std::uint16_t palBank)
{
    const auto dalPath = dataDirectory / ("DALLE" + std::to_string(dalleBank) + ".DAL");
    const auto palPath = dataDirectory / ("PAL" + std::to_string(palBank) + ".PAL");
    const auto palette = loadPalette(palPath);
    // Original DAL addressing uses fixed 4440-byte records for every bank.
    // DALLE5 contains 35 records; the other original banks contain 54.
    const auto bytes = readExactFile(dalPath, dalleBank == 5 ? 35 * imageBytes : bankBytes);
    Bank bank;
    bank.textures.reserve(bytes.size() / imageBytes);
    for (std::size_t start = 0; start < bytes.size(); start += imageBytes) {
        Texture texture{width, height, {}};
        texture.pixels.reserve(imageBytes);
        // DAL stores complete top-to-bottom columns; expose ordinary row-major RGB.
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                texture.pixels.push_back(palette[bytes[start + x * height + y]]);
            }
        }
        bank.textures.push_back(std::move(texture));
    }
    return bank;
}
} // namespace wall_textures
