#include <string_view>
#include <cstdint>
#include "extremis/resources/vga_palette.h"

#include <stdexcept>
#include <string>

namespace vga_palette {
Palette decode(const std::uint8_t* bytes, std::string_view invalidComponent)
{
    Palette palette{};
    for (auto& color : palette) {
        for (auto& component : color) {
            const auto value = *bytes++;
            if (value > 63) {
                throw std::runtime_error(std::string(invalidComponent));
            }
            component = static_cast<std::uint8_t>((value * 255 + 31) / 63);
        }
    }
    return palette;
}
}
