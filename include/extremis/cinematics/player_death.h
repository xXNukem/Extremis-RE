#pragma once
#include "extremis/resources/anm_format.h"
#include "extremis/resources/spr_format.h"
#include "extremis/gameplay/player/player_vitals.h"
#include <string>

namespace player {
struct DeathResources {
    anm_format::Animation health, oxygen, gameOver, explosion;
    std::vector<spr_format::Image> body, font;
    std::vector<std::string> lines;
};
DeathResources loadDeathResources(const std::filesystem::path& directory);
// Recovered iteration counts, not fixed-second movie durations. A presentation
// pulse substitutes for DOS retrace/unpaced iterations (explicit host adaptation).
class DeathPresentation {
public:
    enum class Stage { Inactive, Tint, Body, MovieReveal, Movie, Hold, Fade,
                       GameOverReveal, Text, Wait, GameOverFade, Finished };
    explicit DeathPresentation(DeathResources resources);
    void begin(DeathCause cause, const std::vector<std::uint32_t>& background);
    void reset();
    void loadExplosion(const std::filesystem::path& directory);
    DeathCause cause() const;
    void advance(double seconds, bool keyPressed=false, bool keyHeld=false);
    Stage stage() const;
    bool active() const;
    bool takeSoundStart();
    bool takeSoundStop();
    const std::vector<std::uint32_t>& pixels() const;
private:
    void compose();
    void enter(Stage stage);
    DeathResources resources_;
    DeathCause cause_=DeathCause::None;
    Stage stage_=Stage::Inactive;
    unsigned step_=0, frame_=0, characters_=0;
    double ticks_=0;
    bool soundStart_=false, soundStop_=false, released_=false;
    std::vector<std::uint32_t> background_,pixels_;
};
}
