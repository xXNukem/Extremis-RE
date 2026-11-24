#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <vector>
#include "extremis/resources/anm_format.h"
#include "extremis/resources/spr_format.h"
#include <string>

namespace original_shuttle {
enum class Scene { Startup, Hangar, Nav3, Nav4, Explosion, Cruise, Nav7, Nav8, Arrival, Base, Done };
struct Stage { const char* visual; const char* sound; unsigned frames, interval; };
const Stage& stage(Scene scene);
struct Resources {
    std::array<anm_format::Animation,10> visuals;
    std::array<std::vector<spr_format::Image>,4> sprites; // INST, INS2, SEQ2, INBASE
    std::array<std::string,4> text;
};
Resources loadResources(const std::filesystem::path& directory);
class Presentation {
public:
    explicit Presentation(Resources resources,bool inheritedAnmEscape=false);
    // Independent PIT, retrace and CPU composition clocks. Never skip frames.
    void step(unsigned ticks,bool retrace,bool compose,bool escape=false);
    void advance(double seconds,bool escape=false);
    Scene scene() const;
    unsigned frame() const;
    bool escapeEnabled() const;
    const std::vector<std::uint32_t>& pixels() const;
    const std::vector<std::uint8_t>& indices() const;
    // Stage numbers start voice 0 at volume 48; -1 explicitly stops it.
    std::vector<int> takeCues();
private:
    void enter(Scene scene);
    void draw();
    void callback();
    void colors(unsigned brightness=100);
    void sprite(unsigned bank,int number,int x,int y,int w=-1,int h=-1);
    Resources resources_;
    Scene scene_=Scene::Startup;
    bool escape_=false,cut_=false,waiting_=false,textDone_=false,tick_=true,fade_=false;
    unsigned frame_=0,ticks_=0,total_=0,script_=0,characters_=1,hangarStep_=0,c_=0,delta_=0,brightness_=100;
    int y_=150,pose_=1,direction_=1;
    std::array<int,9> states_{};
    struct Star {int x,z,y;};
    std::array<Star,200> stars_{};
    std::array<std::uint32_t,256> palette_{};
    std::vector<std::uint8_t> canvas_,background_;
    std::vector<std::uint32_t> pixels_=std::vector<std::uint32_t>(64000,255);
    std::vector<int> cues_;
    double pit_=0,retrace_=0,cpu_=0;
};
}
