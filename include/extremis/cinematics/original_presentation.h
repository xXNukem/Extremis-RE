#pragma once
#include <cstdint>
#include <filesystem>
#include <vector>
#include "extremis/resources/anm_format.h"
#include "extremis/resources/spr_format.h"
#include <array>

namespace original_presentation {
struct Resources {
    std::array<std::uint32_t,256> conceptPalette{};
    spr_format::Image conceptSprite;
    anm_format::Animation concept, title;
    // PRESENTE, WHO1, WHO2, WHO3, WHO4.
    std::array<anm_format::Animation,5> cards;
    std::vector<spr_format::Image> creditFont;
};
Resources loadResources(const std::filesystem::path& directory);
enum class Screen { LogoIn, LogoMovie, LogoHold, LogoOut, Presente, Title,
                    Who1, Who2, Who3, Who4, Done };
enum class Phase { Scale, Movie, FadeIn, Hold, FadeOut };
// Nominal PIT elapsed time and 70 Hz host retraces are adaptations. No audio.
// Keys are consumed only at still-card holds; movies/zoom/fades cannot be skipped.
class Presentation {
public:
    explicit Presentation(Resources resources);
    void advance(double seconds, bool key=false);
    Screen screen() const;
    Phase phase() const;
    unsigned frame() const;
    unsigned pass() const;
    unsigned brightness() const;
    const std::vector<std::uint8_t>& indices() const;
    const std::vector<std::uint32_t>& pixels() const;
private:
    void enter(Screen screen);
    void logo(int t, bool scaled);
    void colors();
    const anm_format::Animation& movie() const;
    Resources resources_;
    Screen screen_=Screen::LogoIn;
    Phase phase_=Phase::Scale;
    unsigned frame_=0,pass_=1,brightness_=0,retraces_=0;
    std::uint16_t ticks_=0;
    double pitFraction_=0,retraceFraction_=0;
    bool pendingKey_=false;
    std::array<std::uint32_t,256> palette_{};
    std::vector<std::uint8_t> canvas_=std::vector<std::uint8_t>(64000);
    std::vector<std::uint32_t> pixels_=std::vector<std::uint32_t>(64000,255);
};
}
