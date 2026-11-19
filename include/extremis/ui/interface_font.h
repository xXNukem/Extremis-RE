#pragma once
#include <cstdint>
#include <filesystem>
#include <string_view>
#include <vector>

namespace interface_font {
void validate(const std::vector<std::uint8_t>& font, unsigned height);
std::vector<std::uint8_t> load(const std::filesystem::path& path, unsigned height);
// Original 3-bit glyph rows, four-pixel advance, on a 320x200 RGBA framebuffer.
void draw(std::vector<std::uint32_t>& pixels, const std::vector<std::uint8_t>& font,
          std::uint32_t color, std::string_view text, int x, int y);
}
