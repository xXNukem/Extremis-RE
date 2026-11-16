#pragma once
#include "anm_format.h"
#include "spr_format.h"

namespace original_elevator {
struct Resources { anm_format::Animation background; spr_format::Image sprite; };
Resources loadResources(const std::filesystem::path& directory);
struct Rectangle { int x,y,w,h; };
Rectangle rectangle(int q);
enum class Phase { Initial, FadeIn, Move, Hold, FadeOut, Done };
// One step is one VGA retrace; no gameplay or input ownership.
class Presentation {
public:
    Presentation(Resources resources,int floorDelta);
    void step();
    Phase phase() const { return phase_; }
    int q() const { return drawnQ_; }
    int brightness() const { return brightness_; }
    bool motor() const { return motor_; }
    const std::vector<std::uint8_t>& indices() const { return canvas_; }
    const std::vector<std::uint32_t>& pixels() const { return pixels_; }
private:
    void draw(int q);
    void colors();
    Resources resources_;
    Phase phase_=Phase::Initial;
    int start_=0,end_=0,drawnQ_=0,brightness_=0,fade_=0,hold_=0;
    std::int16_t q_=0,speed_=4;
    bool descending_=false,motor_=false;
    std::vector<std::uint8_t> canvas_;
    std::vector<std::uint32_t> pixels_;
};
}
