#pragma once
#include "anm_format.h"
#include "spr_format.h"
#include <array>

namespace original_ending {
enum class Scene { Gold, Fseq1, Fseq2, Fseq3, Fseq4Move, Fseq4Poses, Fseq5, Fseq6, Hold, Fade, Done };
enum class Phase { Tint, Cut, Frame, Reveal, Wait, Move, Hold, Fade, Done };
enum class Sound { None, Monster, Wind, Labo, Barge, Radar, Count };
struct Cue { unsigned voice; Sound sound; unsigned volume; };
struct Resources {
    // FSEQ1, FSEQ2_1, FSEQ3, FSEQ4_1, FSEQ5, FSEQ6.
    std::array<anm_format::Animation,6> visuals;
    std::array<std::vector<spr_format::Image>,2> sprites;
};
struct EntryImage {
    // Port framebuffer may contain more than 256 colors; entry tint only.
    std::vector<std::uint32_t> rgba;
    std::array<std::uint32_t,256> palette{}; // RGBA8888; quantized back to six-bit DAC.
    std::vector<std::uint8_t> indices=std::vector<std::uint8_t>(64000);
};
Resources loadResources(const std::filesystem::path& directory);
const char* soundFile(Sound sound);
class Presentation {
public:
    explicit Presentation(Resources resources, EntryImage entry={});
    // Escape is deliberately ignored; window/application quit belongs to the host.
    void advance(double seconds, bool escape=false);
    // Independent clock inputs also allow exact threshold tests without wall time.
    // At most one movie frame/retrace operation per call; no catch-up rendering.
    void step(std::uint16_t elapsedTicks, bool retrace);
    Scene scene() const { return scene_; }
    Phase phase() const { return phase_; }
    unsigned frame() const { return frame_; }
    unsigned pass() const { return pass_; }
    unsigned pose() const { return pose_; } // FSEQ2 draw 0..47 / FSEQ4 pose 0..12.
    unsigned amount() const { return amount_; } // tint amount / fade brightness
    const std::vector<std::uint8_t>& indices() const { return canvas_; }
    const std::vector<std::uint32_t>& pixels() const { return pixels_; }
    std::vector<Cue> takeCues();
private:
    void enter(Scene scene);
    void presentFrame();
    void compose();
    void colors(bool blend=false, bool gold=false);
    void sprite(unsigned bank,unsigned number,int x,int y);
    unsigned visual() const;
    unsigned interval() const;
    void cue(unsigned voice,Sound sound=Sound::None,unsigned volume=0);
    std::vector<std::uint32_t> entryPixels_;
    Resources resources_;
    Scene scene_=Scene::Gold;
    Phase phase_=Phase::Tint;
    unsigned frame_=0,pass_=1,pose_=0,amount_=0,retraces_=0;
    std::uint16_t ticks_=0,motionTime_=0;
    double pitFraction_=0,retraceFraction_=0;
    std::array<std::uint32_t,256> palette_{};
    std::vector<std::uint8_t> canvas_;
    std::vector<std::uint32_t> pixels_=std::vector<std::uint32_t>(64000,255);
    std::vector<Cue> cues_;
};
}
