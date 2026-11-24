#include <cstddef>
#include <array>
#include <cstdint>
#include <filesystem>
#include <vector>
#include "extremis/world/pit_clock.h"
#include "extremis/cinematics/original_ending.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace original_ending {
namespace {
constexpr unsigned counts[]{16,1,12,1,52,12};
// EXE 0x21823..0x21856, DOS sprites 1..13.
constexpr int positions[][2]{{77,24},{78,24},{78,24},{79,23},{81,23},{82,23},{85,23},
    {87,22},{90,22},{84,22},{77,21},{69,21},{62,21}};
int signedWord(std::uint16_t v) { return v<32768?int(v):int(v)-65536; }
}
Resources loadResources(const std::filesystem::path& directory) {
    Resources r;
    constexpr const char* names[]{"FSEQ1.ANM","FSEQ2_1.VGA","FSEQ3.ANM","FSEQ4_1.VGA","FSEQ5.ANM","FSEQ6.ANM"};
    for(unsigned i=0;i<6;++i)
        r.visuals[i]=(i==1 || i==3)?anm_format::loadVga(directory/names[i]):anm_format::load(directory/names[i]);
    r.sprites[0]=spr_format::load(directory/"FSEQ2_2.SPR");
    r.sprites[1]=spr_format::load(directory/"FSEQ4_2.SPR");
    return r;
}
const char* soundFile(Sound sound) {
    constexpr const char* names[]{"","MONSTRE.DSS","VENT.DSS","LABO.DSS","BARGE2.DSS","RADAR.DSS"};
    return names[unsigned(sound)];
}
Presentation::Presentation(Resources resources,EntryImage entry)
    :entryPixels_(std::move(entry.rgba)),resources_(std::move(resources)),palette_(entry.palette),canvas_(std::move(entry.indices)) {
    if(!entryPixels_.empty() && entryPixels_.size()!=64000) throw std::runtime_error("Invalid ending RGBA canvas");
    if(canvas_.size()!=64000) throw std::runtime_error("Invalid ending entry canvas");
    for(unsigned i=0;i<6;++i) {
        const auto& a=resources_.visuals[i];
        if(a.frames.size()!=counts[i]) throw std::runtime_error("Unsupported FSEQ frame count");
        for(const auto& f:a.frames) if(f.size()!=64000) throw std::runtime_error("Invalid FSEQ canvas");
    }
    for(unsigned bank=0;bank<2;++bank) {
        if(resources_.sprites[bank].size()!=(bank?13u:7u)) throw std::runtime_error("Unsupported FSEQ sprite count");
        for(const auto& s:resources_.sprites[bank])
            if(s.width<=0 || s.height<=0 || s.width>320 || s.height>200 || s.anchorX || s.anchorY ||
               s.indices.size()!=std::size_t(s.width)*s.height)
                throw std::runtime_error("Invalid FSEQ sprite");
    }
    colors(true,true);
}
unsigned Presentation::visual() const {
    switch(scene_) {
    case Scene::Fseq1:return 0;
    case Scene::Fseq2:return 1;
    case Scene::Fseq3:return 2;
    case Scene::Fseq4Move:case Scene::Fseq4Poses:return 3;
    case Scene::Fseq5:return 4;
    default:return 5;
    }
}
unsigned Presentation::interval() const { return scene_==Scene::Fseq1?10:scene_==Scene::Fseq5?12:13; }
void Presentation::cue(unsigned voice,Sound sound,unsigned volume) { cues_.push_back({voice,sound,volume}); }
std::vector<Cue> Presentation::takeCues() { auto result=std::move(cues_);cues_.clear();return result; }
void Presentation::enter(Scene scene) {
    scene_=scene;ticks_=0;frame_=0;pose_=0;retraces_=0;motionTime_=0;
    if(scene==Scene::Hold) {phase_=Phase::Hold;cue(1,Sound::Radar,48);return;}
    if(scene==Scene::Fade) {phase_=Phase::Fade;amount_=100;return;}
    if(scene==Scene::Done) {
        phase_=Phase::Done;cue(1);cue(0);canvas_.assign(64000,0);pixels_.assign(64000,255);return;
    }
    if(scene==Scene::Fseq4Poses) {phase_=Phase::Wait;compose();return;}
    if(scene==Scene::Fseq1) {
        cue(0,Sound::Monster,64);phase_=Phase::Frame;return; // Retain gold DAC until reveal.
    }
    cue(0);
    switch(scene) {
    case Scene::Fseq2:cue(0,Sound::Wind,48);break;
    case Scene::Fseq3:cue(0,Sound::Wind,32);break;
    case Scene::Fseq4Move:cue(0,Sound::Labo,48);break;
    case Scene::Fseq5:cue(0,Sound::Barge,48);break;
    case Scene::Fseq6:cue(0,Sound::Radar,48);break;
    default:break;
    }
    // Explicit black boundary, but no invented DOS disk-loading duration.
    phase_=Phase::Cut;canvas_.assign(64000,0);pixels_.assign(64000,255);
}
void Presentation::colors(bool blend,bool gold) {
    std::array<std::uint32_t,256> colors{};
    const unsigned target[]{gold?63u:0u,gold?48u:0u,0};
    const auto convert=[&](std::uint32_t pixel) {
        std::uint32_t rgb=0;unsigned channel=0;
        for(int shift:{24,16,8}) {
            const unsigned c=(((pixel>>shift)&255)*63+127)/255;
            // EXE 0x14611: separate target truncation and 16.16 source lookup.
            const unsigned dac=blend?(target[channel]*amount_/100+
                (amount_==0?c:(c*((65536u*(100-amount_))/100))>>16)):c*amount_/100;
            rgb=(rgb<<8)|((dac*255+31)/63);++channel;
        }
        return (rgb<<8)|255;
    };
    if(scene_==Scene::Gold && !entryPixels_.empty()) {
        for(unsigned i=0;i<64000;++i) pixels_[i]=convert(entryPixels_[i]);
        return;
    }
    for(unsigned i=0;i<256;++i) colors[i]=convert(palette_[i]);
    for(unsigned i=0;i<64000;++i) pixels_[i]=colors[canvas_[i]];
}
void Presentation::sprite(unsigned bank,unsigned number,int x,int y) {
    const auto& s=resources_.sprites[bank][number-1];
    // All FSEQ anchors are zero. Clipping is needed during FSEQ4 translation;
    // bounds checks also keep malformed external data from writing off-canvas.
    for(int py=std::max(0,y);py<std::min(200,y+s.height);++py)
        for(int px=std::max(0,x);px<std::min(320,x+s.width);++px) {
            const auto c=s.indices[std::size_t(py-y)*s.width+px-x];
            if(c) canvas_[py*320+px]=c;
        }
}
void Presentation::compose() {
    const auto& a=resources_.visuals[visual()];palette_=a.palette;canvas_=a.frames[0];amount_=100;
    if(scene_==Scene::Fseq2) {
        const unsigned p=pose_%12;
        sprite(0,p<=6?p+1:13-p,154,102+int(pose_));
    } else if(scene_==Scene::Fseq4Move) sprite(1,1,274-197*signedWord(motionTime_)/320,24);
    else sprite(1,pose_+1,positions[pose_][0],positions[pose_][1]);
    colors();
}
void Presentation::presentFrame() {
    const auto& a=resources_.visuals[visual()];canvas_=a.frames[frame_];palette_=a.palette;
    if(frame_==0 && ((scene_==Scene::Fseq1 && pass_==1) || scene_==Scene::Fseq6)) {
        phase_=Phase::Reveal;amount_=100;colors(true,scene_==Scene::Fseq1);
    } else {phase_=Phase::Wait;amount_=100;colors();}
}
void Presentation::advance(double seconds,bool /*escape*/) {
    if(!std::isfinite(seconds) || seconds<0 || scene_==Scene::Done) return;
    constexpr double rate=extremis::pit::rate;
    pitFraction_+=std::fmod(seconds,65536.0/rate)*rate;
    const double whole=std::floor(pitFraction_);pitFraction_-=whole;
    retraceFraction_+=std::min(seconds,1.0)*70;
    const bool retrace=retraceFraction_>=1;
    if(retrace) retraceFraction_=std::fmod(retraceFraction_,1.0);
    step(std::uint16_t(std::fmod(whole,65536.0)),retrace);
}
void Presentation::step(std::uint16_t elapsedTicks,bool retrace) {
    if(scene_==Scene::Done) return;
    ticks_=std::uint16_t(ticks_+elapsedTicks);
    if(phase_==Phase::Tint) {
        if(!retrace) return;
        colors(true,true);
        if(amount_==100) enter(Scene::Fseq1);else amount_+=4;
        return;
    }
    if(phase_==Phase::Cut) {
        ticks_=0;
        if(scene_==Scene::Fseq2) {phase_=Phase::Wait;compose();return;}
        phase_=scene_==Scene::Fseq4Move?Phase::Move:Phase::Frame;
    }
    if(phase_==Phase::Frame) {if(retrace) presentFrame();return;}
    if(phase_==Phase::Reveal) {
        if(!retrace) return;
        colors(true,scene_==Scene::Fseq1);
        if(amount_==0) phase_=Phase::Wait;else amount_-=4;
        return; // Reveal shares the movie's initial counter, never resets it.
    }
    if(phase_==Phase::Hold) {
        if(retrace && ++retraces_==350) enter(Scene::Fade);
        return;
    }
    if(phase_==Phase::Fade) {
        if(!retrace) return;
        colors();
        if(amount_==0) enter(Scene::Done);else --amount_;
        return;
    }
    if(phase_==Phase::Move) {
        if(!retrace) return;
        compose(); // Draw at previous t, then accumulate the time spent awaiting retrace.
        motionTime_=std::uint16_t(motionTime_+ticks_);ticks_=0;
        if(signedWord(motionTime_)>=320) enter(Scene::Fseq4Poses);
        return;
    }
    if(scene_==Scene::Fseq2 || scene_==Scene::Fseq4Poses) {
        if(scene_==Scene::Fseq2?signedWord(ticks_)<=10:signedWord(ticks_)<32) return;
        ticks_=0;++pose_;
        if(pose_==(scene_==Scene::Fseq2?48u:13u)) enter(scene_==Scene::Fseq2?Scene::Fseq3:Scene::Fseq5);
        else compose();
        return;
    }
    if(signedWord(ticks_)<=int(interval())) return;
    ticks_=0;
    if(++frame_==resources_.visuals[visual()].frames.size()) {
        if(scene_==Scene::Fseq1 && pass_<4) {++pass_;frame_=0;phase_=Phase::Frame;}
        else {
            enter(scene_==Scene::Fseq1?Scene::Fseq2:scene_==Scene::Fseq3?Scene::Fseq4Move:
                  scene_==Scene::Fseq5?Scene::Fseq6:Scene::Hold);
            return;
        }
    } else phase_=Phase::Frame;
    if(retrace) presentFrame();
}
}

namespace original_ending {
Scene Presentation::scene() const
{
    return scene_;
}

Phase Presentation::phase() const
{
    return phase_;
}

unsigned Presentation::frame() const
{
    return frame_;
}

unsigned Presentation::pass() const
{
    return pass_;
}

unsigned Presentation::pose() const
{
    return pose_;
}

unsigned Presentation::amount() const
{
    return amount_;
}

const std::vector<std::uint8_t>& Presentation::indices() const
{
    return canvas_;
}

const std::vector<std::uint32_t>& Presentation::pixels() const
{
    return pixels_;
}
} // namespace original_ending
