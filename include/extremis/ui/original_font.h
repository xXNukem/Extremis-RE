#pragma once

#include <SDL3/SDL.h>
#include <array>
#include <filesystem>
#include <string_view>

// Authentic In Extremis message glyphs from the player's own DATA/CASQUE.SPR.
// No original game assets are embedded or redistributed with the source port.
// Reusable by introductory credits and future SDL menu screens.
namespace original_font {
class Font {
public:
    explicit Font(SDL_Renderer* renderer, const std::filesystem::path& casqueSpr);
    ~Font();
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    // Width in original font pixels. Height is always 10 pixels per line.
    float measure(std::string_view text) const;
    bool draw(SDL_Renderer* renderer, std::string_view text, float x, float y,
              float scale, SDL_Color tint, float opacity = 1.f) const;
    bool drawCentered(SDL_Renderer* renderer, std::string_view text, float centerX,
                      float y, float scale, SDL_Color tint, float opacity = 1.f) const;

private:
    struct Glyph { SDL_Texture* texture = nullptr; int width = 0; };
    std::array<Glyph, 44> glyphs_{}; // CASQUE sprite indices 42..85.
};
}
