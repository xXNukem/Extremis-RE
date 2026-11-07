#pragma once
#include "spr_format.h"
#include "anm_format.h"
#include "texture_bank.h"
#include <array>
#include <optional>
#include <string>
#include <string_view>

namespace interaction {
enum class Screen { Code, Supply, Weapon, Cards, Reader, Plan, Codes, Inventory, Elevator };
enum class Phase { Ready, Opening, HandIn, CodeResult, HandOut, PanelSlide, PanelZoom,
                   Shutters, ContentExpand, ContentMove, ContentReveal, Closing,
                   CardInsert, CardCheck, CardResult, CardWithdraw, MovieIn, Movie, MovieOut };
struct State {
    Phase phase = Phase::Ready;
    unsigned frame = 0;
    double movieSeconds = 0;
    bool contentTaken = false;
    bool codeAccepted = false, cardAccepted = false;
    Screen screen = Screen::Code;
    std::uint16_t contents = 0;
    unsigned selection = 0;
    std::string entered, notice;
    std::array<int,3> cards{-1,-1,-1};
    bool blinkVisible = true;
    bool replacing = false;
    unsigned transfer = 100;
    std::optional<unsigned> pressedKey;
    std::array<int,2> injectors{-1,-1};
    int oxygenReserve = -1;
    int batteryCharge = -1;
    bool detectorEnabled = false;
    bool inventoryUse = false;
    unsigned useSelection = 0;
    unsigned fingerPhase = 0;
    bool elevator = false;
    unsigned elevatorIndex = 0;
    unsigned elevatorZone = 0, elevatorSector = 0;
    bool codeReader = false;
    unsigned codeIndex = 0;
};
struct Resources {
    anm_format::Animation pickupMovie;
    std::vector<spr_format::Image> keyboard, supplies, weapons, inventory, letters, reader, plan, codes, elevator;
    std::vector<spr_format::Image> supplyDoor, supplyOpening, weaponDoor, weaponOpening, planDoor;
    std::vector<std::uint8_t> smallFont, tinyFont;
    std::array<wall_textures::Rgb,256> palette{};
};
Resources load(const std::filesystem::path& directory);
class Renderer {
public:
    explicit Renderer(Resources resources);
    bool draw(std::vector<std::uint32_t>& pixels, const State& state) const;
private:
    void image(std::vector<std::uint32_t>& pixels, const spr_format::Image& sprite,
               int x, int y, bool transparent = true, int clipTop = 0, int clipBottom = 200,
               int clipLeft = 0, int clipRight = 320) const;
    void smallText(std::vector<std::uint32_t>& pixels, std::string_view text, int x, int y, bool tiny = false) const;
    void menuText(std::vector<std::uint32_t>& pixels, std::string_view text, int y, bool selected) const;
    void movie(std::vector<std::uint32_t>& pixels, const State& state) const;
    bool sequence(std::vector<std::uint32_t>& pixels, const State& state) const;
    void shutters(std::vector<std::uint32_t>& pixels, const State& state) const;
    void scaled(std::vector<std::uint32_t>& pixels, const spr_format::Image& sprite,
                int x, int y, int width, int height) const;
    void reader(std::vector<std::uint32_t>& pixels, const State& state) const;
    void code(std::vector<std::uint32_t>& pixels, const State& state) const;
    void supply(std::vector<std::uint32_t>& pixels, const State& state, bool weapon) const;
    void inventory(std::vector<std::uint32_t>& pixels, const State& state) const;
    void cards(std::vector<std::uint32_t>& pixels, const State& state) const;
    Resources resources_;
    std::array<std::uint32_t,256> colors_{};
};
std::optional<std::uint8_t> codeChecksum(std::string_view code);
std::string cardName(int packed);
}
