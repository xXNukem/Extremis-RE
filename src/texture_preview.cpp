#include "texture_preview.h"

#include <exception>
#include <iostream>

void TexturePreview::load(const std::filesystem::path& dataDirectory, std::uint16_t dalleBank,
                          std::uint16_t palBank)
{
    bank_ = {};
    overlays_ = {};
    animationClock_ = {};
    selected_ = 0;
    manualOverride_ = false;
    try {
        bank_ = wall_textures::load(dataDirectory, dalleBank, palBank);
        const auto& texture = bank_.textures.front();
        std::cout << "Loaded DALLE" << dalleBank << " / PAL" << palBank << ": "
                  << bank_.textures.size() << " textures, " << texture.width << 'x'
                  << texture.height << ". Original wall selection: type & 0xFF.\n"
                  << "N/M: override with a PROVISIONAL single-texture preview.\n";
    } catch (const std::exception& error) {
        std::cerr << "Wall textures unavailable: " << error.what()
                  << ". Using flat wall colors.\n";
    }
    if (bank_.textures.empty()) return;
    try {
        overlays_ = wall_textures::loadWallOverlays(dataDirectory, palBank);
        wall_animation::compose(bank_, overlays_, animationClock_.tick());
        std::cout << "Animated wall materials: 11, 12, 13 (ADALLE). "
                     "Global tick: provisional 100 ms.\n";
    } catch (const std::exception& error) {
        overlays_ = {};
        std::cerr << "Wall animations unavailable: " << error.what()
                  << ". Keeping static DALLE materials.\n";
    }
}

void TexturePreview::update(double elapsedSeconds)
{
    if (!overlays_.textures.empty() && animationClock_.advance(elapsedSeconds)) {
        wall_animation::compose(bank_, overlays_, animationClock_.tick());
    }
}

void TexturePreview::changeIndex(int direction)
{
    if (bank_.textures.empty() || direction == 0) {
        return;
    }
    const auto count = bank_.textures.size();
    manualOverride_ = true;
    selected_ = direction > 0 ? (selected_ + 1) % count : (selected_ + count - 1) % count;
    std::cout << "PROVISIONAL wall texture: " << selected_ << '/' << count - 1 << '\n';
}

const wall_textures::Texture* TexturePreview::selectedTexture() const
{
    return bank_.textures.empty() || !manualOverride_ ? nullptr : &bank_.textures[selected_];
}

const wall_textures::Bank* TexturePreview::materials() const
{
    return bank_.textures.empty() ? nullptr : &bank_;
}

std::string TexturePreview::windowTitle() const
{
    if (bank_.textures.empty()) {
        return "In Extremis Port | flat wall colors";
    }
    if (!manualOverride_) {
        return "In Extremis Port | original wall materials | TAB: 2D/3D | N/M: preview";
    }
    return "In Extremis Port | PROVISIONAL texture " + std::to_string(selected_) + '/' +
           std::to_string(bank_.textures.size() - 1) + " | N/M: select | TAB: 2D/3D";
}
