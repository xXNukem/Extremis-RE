#pragma once

#include <SDL3/SDL_render.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <string_view>

namespace startup_credits {
class CreditsFont {
public:
    CreditsFont() = default;
    CreditsFont(const CreditsFont&) = delete;
    CreditsFont& operator=(const CreditsFont&) = delete;
    ~CreditsFont();

    bool load(SDL_Renderer* renderer, const std::filesystem::path& path);
    float advance(unsigned char c) const;
    float extent(std::string_view text) const;
    bool glyph(SDL_Renderer* renderer, unsigned char c, float x, float y, float scale,
               Uint8 red, Uint8 green, Uint8 blue, float alpha) const;

private:
    void clear();
    static int index(unsigned char c);

    std::array<SDL_Texture*,44> glyphs{};
    std::array<int,44> glyphWidths{};
};

std::uint32_t mixBits(std::uint32_t value);
bool drawCredit(SDL_Renderer* renderer, const CreditsFont* font, double time, std::uint32_t launchSeed);
}
