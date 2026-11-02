#pragma once

#include "texture_bank.h"

#include <cstddef>
#include <string>

// Original wall materials by default; N/M retain the provisional single-image preview.
class TexturePreview {
public:
    void load(const std::filesystem::path& dataDirectory, std::uint16_t dalleBank,
              std::uint16_t palBank);
    void changeIndex(int direction);
    const wall_textures::Texture* selectedTexture() const;
    const wall_textures::Bank* materials() const;
    std::string windowTitle() const;

private:
    wall_textures::Bank bank_;
    std::size_t selected_ = 0;
    bool manualOverride_ = false;
};
