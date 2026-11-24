#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include "extremis/resources/anm_format.h"
#include "extremis/resources/spr_format.h"
#include "extremis/resources/game_lab.h"
#include "extremis/cinematics/playback_result.h"

#include <filesystem>

struct SDL_Window;
#include <functional>
#include <string>

namespace communication {
unsigned doorFade(const game_lab::Level&,unsigned sector);
bool trigger(game_lab::Level&,unsigned sector);
inline constexpr game_lab::PlayerStart returnPosition{16,500,300,180};
struct Resources {
    anm_format::Animation prompt, first, second;
    std::vector<spr_format::Image> font, sprites;
    std::array<std::string,3> question; // Original question / YES storage / NO.
    std::array<std::string,7> pages;
};
Resources loadResources(const std::filesystem::path& directory);
enum class Phase { PromptIn, Question, Choice, PromptOut, ScreenIn, Opening,
                   Typing, Acknowledge, Closing, ScreenOut, Done };
struct Input { int choice=0; bool action=false,escape=false; };
class Presentation {
public:
    explicit Presentation(Resources);
    void advance(double seconds,Input input={});
    Phase phase() const;
    unsigned page() const;
    unsigned characters() const;
    unsigned brightness() const;
    bool yes() const;
    const std::vector<std::uint32_t>& pixels() const;
private:
    void enter(Phase);
    void beginPage(bool escape);
    void animate();
    void render();
    Resources resources_;
    Phase phase_=Phase::PromptIn;
    unsigned page_=0,characters_=0,brightness_=0,frame_=70,portrait_=0;
    int scan_=11,scanDelta_=4;
    bool yes_=true;
    double pulse_=0,pitFraction_=0;
    unsigned wait_=0,blink_=0;
    std::array<std::vector<std::uint8_t>,2> screens_;
    std::vector<std::uint32_t> pixels_=std::vector<std::uint32_t>(64000,255);
};
// Reuse the caller's framebuffer presentation; no new renderer/audio ownership.
StartupCreditsResult show(SDL_Window*,const std::filesystem::path&,
    const std::function<bool(const std::vector<std::uint32_t>&)>& present);
}
