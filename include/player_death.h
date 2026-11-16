#pragma once
#include "anm_format.h"
#include "spr_format.h"
#include "player_vitals.h"
#include <string>

namespace player {
struct DeathResources {
    anm_format::Animation health, oxygen, gameOver, menu, explosion;
    std::vector<spr_format::Image> body, font, menuFont, menuArt;
    std::vector<std::string> lines;
    std::vector<std::string> menuLines;
};
DeathResources loadDeathResources(const std::filesystem::path& directory);
// Recovered iteration counts, not fixed-second movie durations. A presentation
// pulse substitutes for DOS retrace/unpaced iterations (explicit host adaptation).
class DeathPresentation {
public:
    enum class Stage { Inactive, Tint, Body, MovieReveal, Movie, Hold, Fade,
                       GameOverReveal, Text, Wait, GameOverFade, Finished, MenuReveal, Menu };
    enum class Request { None, Restart, Exit, Unsupported };
    explicit DeathPresentation(DeathResources resources);
    void begin(DeathCause cause, const std::vector<std::uint32_t>& background);
    void reset();
    void loadExplosion(const std::filesystem::path& directory);
    DeathCause cause() const { return cause_; }
    void beginMenu();
    void advance(double seconds, bool keyPressed=false, bool keyHeld=false);
    Request menuInput(int direction, bool confirm);
    Stage stage() const { return stage_; }
    bool active() const { return stage_!=Stage::Inactive; }
    bool takeSoundStart();
    bool takeSoundStop();
    const std::vector<std::uint32_t>& pixels() const { return pixels_; }
private:
    void compose();
    void enter(Stage stage);
    DeathResources resources_;
    DeathCause cause_=DeathCause::None;
    Stage stage_=Stage::Inactive;
    unsigned step_=0, frame_=0, characters_=0;
    double ticks_=0;
    bool soundStart_=false, soundStop_=false, released_=false;
    unsigned selection_=0;
    bool runMenu_=false;
    std::vector<std::uint32_t> background_,pixels_;
};
}
