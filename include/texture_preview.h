#pragma once

#include "texture_bank.h"
#include "wall_animation.h"

#include <cstddef>
#include <string>

// Original wall materials by default; N/M retain the provisional single-image preview.
class TexturePreview {
public:
    void load(const std::filesystem::path& dataDirectory, std::uint16_t dalleBank,
              std::uint16_t palBank);
    void changeIndex(int direction);
    void update(double elapsedSeconds);
    const wall_textures::Texture* selectedTexture() const;
    const wall_textures::Bank* materials() const;
    std::string windowTitle() const;

private:
    wall_textures::Bank bank_;
    wall_textures::Bank overlays_;
    wall_animation::Clock animationClock_;
    std::size_t selected_ = 0;
    bool manualOverride_ = false;
};
