#pragma once
#include <cstdint>
#include <filesystem>
#include <vector>
#include "extremis/resources/anm_format.h"
#include "extremis/resources/spr_format.h"
#include <array>
#include <string>

namespace original_intro {
enum class Visual { Ship, Cockpit, CockpitFlash, Radar, Labo, Message, Stars,
                    Alarm, Panel, Depart, Transfer, Explosion, ExplosionFlash, Count };
enum class Bank { Cockpit, Message, Stars, Panel, Hand, Depart, Transfer, Explosion, Font, Count };
enum class Sound { None, Barge, Cabin, Radar, Labo, Alarm1, Alarm4, Touch, Oxygen, Explosion, Count };
enum class Scene { Ship, Cockpit, Radar, Labo, CockpitAlarm, Message, Stars,
                   Alarm, Panel, Depart, Transfer, Explosion, Fade, AudioWait, Done };
struct Resources {
    std::array<anm_format::Animation,static_cast<unsigned>(Visual::Count)> visuals;
    std::array<std::vector<spr_format::Image>,static_cast<unsigned>(Bank::Count)> sprites;
    std::array<std::string,2> text;
};
Resources loadResources(const std::filesystem::path& directory);
const char* soundFile(Sound sound);
struct Cue { unsigned voice; Sound sound; unsigned volume; }; // None stops that voice.

// Rendering pulses and nominal PIT elapsed time are deliberately separate.
// 70 Hz retrace and the SDL host's 60 Hz unpaced-loop pulses are adaptations,
// not measured DOS timings. advance never catches up multiple movie frames.
class Presentation {
public:
    explicit Presentation(Resources resources, bool skipAtEntry=false);
    void advance(double seconds, bool escape=false, bool voice0Active=false);
    Scene scene() const;
    bool unpaced() const;
    unsigned frame() const;
    unsigned record() const;
    unsigned textPosition() const;
    unsigned moviePass() const;
    const std::vector<std::uint8_t>& indices() const;
    const std::vector<std::uint32_t>& pixels() const;
    std::vector<Cue> takeCues();
private:
    void enter(Scene scene);
    void finish();
    void compose();
    void movieFrame();
    void drawText(unsigned which);
    void sprite(Bank bank, int number, int x, int y, int w=0, int h=0, bool anchors=false);
    void cue(unsigned voice, Sound sound=Sound::None, unsigned volume=0);
    void colors();
    Resources resources_;
    Scene scene_=Scene::Ship;
    unsigned frame_=0,record_=0,textPosition_=0,moviePass_=0,brightness_=0;
    std::uint16_t time_=0,phase_=0,wait_=0;
    double pitFraction_=0,retraceFraction_=0;
    bool pendingEscape_=false,revealing_=true,messageSprites_=false,exploded_=false;
    std::array<std::array<int,3>,400> stars_{};
    std::array<std::uint32_t,256> palette_{};
    std::vector<std::uint8_t> canvas_=std::vector<std::uint8_t>(64000);
    std::vector<std::uint32_t> pixels_=std::vector<std::uint32_t>(64000,255);
    std::vector<Cue> cues_;
};
}
