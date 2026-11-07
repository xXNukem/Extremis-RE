#pragma once
#include "camera.h"
#include "spr_format.h"
#include "texture_bank.h"
#include <array>
#include <optional>

namespace original_map {
constexpr unsigned levelCount=30;
struct Resources {
    std::vector<spr_format::Image> maps, compass;
    std::vector<std::uint8_t> font;
    std::array<wall_textures::Rgb,256> palette{};
};
Resources load(const std::filesystem::path& directory);
struct Placement { int x, y, markerX, markerY; unsigned direction; };
std::optional<Placement> placement(int width, int height, const Camera& camera);
struct State { unsigned zone=0; Camera camera; bool markerVisible=false; };
class Renderer {
public:
    explicit Renderer(Resources resources);
    // Consumes the freshly composed, frozen game frame; never changes game palettes.
    bool draw(std::vector<std::uint32_t>& pixels, const State& state) const;
private:
    Resources resources_;
    std::array<std::uint32_t,256> colors_{};
    void image(std::vector<std::uint32_t>& pixels, const spr_format::Image& image, int x, int y) const;
};
}
