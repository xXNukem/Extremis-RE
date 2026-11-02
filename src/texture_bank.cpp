#include "texture_bank.h"

#include <array>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace wall_textures {
namespace {
constexpr int width = 74;
constexpr int height = 60;
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

std::array<Rgb, 256> loadPalette(const std::filesystem::path& path)
{
    const auto bytes = readExactFile(path, 768);
    std::array<Rgb, 256> palette{};
    for (std::size_t i = 0; i < palette.size(); ++i) {
        std::array<std::uint8_t, 3> rgb{};
        for (std::size_t channel = 0; channel < rgb.size(); ++channel) {
            const auto value = bytes[i * 3 + channel];
            if (value > 63) {
                throw std::runtime_error("Invalid 6-bit palette component in " + path.string());
            }
            // Same full-range rounding as the research extractor; 63 gives 255.
            rgb[channel] = static_cast<std::uint8_t>((value * 255 + 31) / 63);
        }
        palette[i] = {rgb[0], rgb[1], rgb[2]};
    }
    return palette;
}
} // namespace

Bank load(const std::filesystem::path& dataDirectory, std::uint16_t dalleBank,
          std::uint16_t palBank)
{
    if (dalleBank != 1) {
        throw std::runtime_error("DALLE" + std::to_string(dalleBank) +
                                 " layout has not been validated; only DALLE1 is supported");
    }
    const auto dalPath = dataDirectory / ("DALLE" + std::to_string(dalleBank) + ".DAL");
    const auto palPath = dataDirectory / ("PAL" + std::to_string(palBank) + ".PAL");
    const auto palette = loadPalette(palPath);
    const auto bytes = readExactFile(dalPath, bankBytes);
    Bank bank;
    bank.textures.reserve(bytes.size() / imageBytes);
    for (std::size_t start = 0; start < bytes.size(); start += imageBytes) {
        Texture texture{width, height, {}};
        texture.pixels.reserve(imageBytes);
        for (std::size_t pixel = 0; pixel < imageBytes; ++pixel) {
            texture.pixels.push_back(palette[bytes[start + pixel]]);
        }
        bank.textures.push_back(std::move(texture));
    }
    return bank;
}
} // namespace wall_textures
