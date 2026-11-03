#pragma once

#include "spr_format.h"
#include "texture_bank.h"
#include "motion_detector.h"

#include <array>
#include <optional>
#include <string_view>

namespace hud {
constexpr int width = 320, height = 200, viewHeight = 150;

// Visual inputs only. The runtime adapter supplies the protagonist's vitals;
// this module owns no health, oxygen, inventory, weapon or message timers.
struct State {
    int health = 1000;       // Original full-scale value.
    int oxygen = 288000;     // Original full-scale value, not seconds.
    std::optional<unsigned> weapon;
    bool firing = false;
    unsigned barrelPhase = 0;
    int ammunition = 0;      // Three decimal digits, when a weapon is supplied.
    std::optional<unsigned> slimePhase; // GOUTTE counter0..29.
    detector::Reading detector;
    int batteryCharge=-1;
    bool detectorEnabled=false;
    bool oxygenBottle = false;
    unsigned cards = 0, injectors = 0; // Occupied original slots, including empty injectors.
    std::string_view message; // Already visible text; no invented typewriter clock.
    std::array<std::string_view,5> messages{};
};

struct Resources {
    std::vector<spr_format::Image> casque;
    std::vector<spr_format::Image> slime;
    std::array<spr_format::Image, 6> weaponIcons{};
    std::array<spr_format::Image, 6> weaponViews{};
    std::array<std::array<spr_format::Image,2>,6> weaponEffects{};
    std::array<wall_textures::Rgb, 256> palette{};
};
Resources load(const std::filesystem::path& directory, unsigned paletteBank = 1);

struct Presentation { float x, y, width, height; };
// VGA 320x200 displayed on a 4:3 screen (original pixels are not square).
Presentation presentation(int outputWidth, int outputHeight);

class Renderer {
public:
    explicit Renderer(Resources resources);
    // Composes onto a complete 320x200 software framebuffer. No allocations.
    // Returns false, without writes, for an invalid framebuffer size.
    bool draw(std::vector<std::uint32_t>& pixels, const State& state) const;
private:
    void image(std::vector<std::uint32_t>& pixels, const spr_format::Image& sprite,
               int x, int y, bool transparent = false, int columns = -1) const;
    void slime(std::vector<std::uint32_t>& pixels, unsigned phase) const;
    void corners(std::vector<std::uint32_t>& pixels) const;
    void bar(std::vector<std::uint32_t>& pixels, int value, int maximum, int y) const;
    void weaponView(std::vector<std::uint32_t>& pixels, const State& state) const;
    void weapon(std::vector<std::uint32_t>& pixels, const State& state) const;
    int message(std::vector<std::uint32_t>& pixels, std::string_view text, int y = 16) const;
    void equipment(std::vector<std::uint32_t>& pixels, const State& state) const;
    Resources resources_;
    std::array<std::uint32_t, 256> colors_{};
};
} // namespace hud
