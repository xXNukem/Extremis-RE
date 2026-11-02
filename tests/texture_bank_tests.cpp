#include "texture_bank.h"
#include "texture_preview.h"

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void write(const std::filesystem::path& path, const std::vector<std::uint8_t>& data)
{
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    check(bool(file), "Cannot write synthetic texture fixture");
}

void expectFailure(const std::filesystem::path& directory, std::uint16_t bank = 1)
{
    try {
        wall_textures::load(directory, bank, 2);
    } catch (const std::runtime_error&) {
        return;
    }
    throw std::runtime_error("Invalid texture input accepted");
}
} // namespace

int main(int argc, char* argv[])
{
    if (argc != 2) {
        return 1;
    }
    const std::filesystem::path directory = argv[1];
    try {
        std::filesystem::create_directories(directory);
        std::vector<std::uint8_t> pixels(239760);
        for (std::size_t i = 0; i < pixels.size(); ++i) {
            pixels[i] = i % 256;
        }
        std::vector<std::uint8_t> palette(768);
        for (std::size_t i = 0; i < 256; ++i) {
            palette[i * 3] = i % 64;
            palette[i * 3 + 1] = 63;
        }
        const auto dalPath = directory / "DALLE1.DAL";
        const auto palPath = directory / "PAL2.PAL";
        write(dalPath, pixels);
        write(palPath, palette);
        const auto bank = wall_textures::load(directory, 1, 2);
        check(bank.textures.size() == 54, "Incorrect texture count");
        for (std::size_t i = 0; i < bank.textures.size(); ++i) {
            const auto& texture = bank.textures[i];
            check(texture.width == 74 && texture.height == 60 && texture.pixels.size() == 4440,
                  "Incorrect texture dimensions");
            for (std::size_t p = 0; p < texture.pixels.size(); ++p) {
                const auto index = pixels[i * 4440 + p];
                const auto rgb = texture.pixels[p];
                check(rgb.r == ((index % 64) * 255 + 31) / 63 && rgb.g == 255 && rgb.b == 0,
                      "Palette / row-major conversion mismatch");
            }
        }
        TexturePreview preview;
        preview.load(directory, 1, 2);
        check(!preview.selectedTexture() && preview.materials() &&
              preview.materials()->textures.size() == 54 &&
              preview.windowTitle().find("original wall materials") != std::string::npos,
              "Original material mode did not load");
        preview.changeIndex(-1);
        check(preview.windowTitle().find("texture 53/") != std::string::npos,
              "Previous texture wrap failed");
        check(preview.selectedTexture() != nullptr, "Manual texture override not enabled");
        preview.changeIndex(1);
        check(preview.windowTitle().find("texture 0/") != std::string::npos,
              "Next texture wrap failed");
        expectFailure(directory, 2);
        auto invalid = palette;
        invalid[0] = 64;
        write(palPath, invalid);
        expectFailure(directory);
        write(palPath, std::vector<std::uint8_t>(767));
        expectFailure(directory);
        write(palPath, palette);
        pixels.pop_back();
        write(dalPath, pixels);
        expectFailure(directory);
        pixels.resize(239761);
        write(dalPath, pixels);
        expectFailure(directory);
        std::filesystem::remove(dalPath);
        expectFailure(directory);
        preview.load(directory, 1, 2);
        check(!preview.selectedTexture() && !preview.materials(),
              "Missing texture did not enable flat-color fallback");
        std::filesystem::remove(palPath);
        expectFailure(directory);
        std::cout << "Texture decoding, selected bank, invalid files and N/M preview passed.\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
