#pragma once

#include <cstdint>

struct SDL_Renderer;
class PortMenuState;
namespace original_font { class Font; }

namespace app::menu_detail {
std::uint32_t hash32(std::uint32_t x);

bool renderPortMenu(SDL_Renderer* renderer, const original_font::Font* font,
                    const PortMenuState& state, double seconds, double selectionAge,
                    std::uint32_t effectSeed, std::uint32_t selectionSeed);
}
