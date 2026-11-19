#include "extremis/world/pit_clock.h"
#include "extremis/cinematics/original_presentation.h"
#include "vortex/core/graphics/color.h"
#include "extremis/resources/texture_bank.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace original_presentation {
Resources loadResources(const std::filesystem::path& directory) {
    Resources r;
    const auto palette=wall_textures::loadPalette(directory/"CONCEPT.PAL");
    for(unsigned i=0;i<256;++i)
        r.conceptPalette[i]=vortex::packOpaqueRgb(palette[i].r, palette[i].g, palette[i].b);
    auto sprites=spr_format::load(directory/"CONCEPT.SPR");
    if(sprites.empty()) throw std::runtime_error("Missing CONCEPT sprite 1");
    r.conceptSprite=std::move(sprites.front());
    r.concept=anm_format::load(directory/"CONCEPT.ANM");
    r.title=anm_format::load(directory/"TITRE.ANM");
    constexpr const char* names[]{"PRESENTE.VGA","WHO1.VGA","WHO2.VGA","WHO3.VGA","WHO4.VGA"};
    for(unsigned i=0;i<5;++i) r.cards[i]=anm_format::loadVga(directory/names[i]);
    return r;
}
Presentation::Presentation(Resources resources):resources_(std::move(resources)) {
    const auto validate=[](const anm_format::Animation& a,unsigned count) {
        if(a.frames.size()!=count) throw std::runtime_error("Unsupported original presentation frame count");
        for(const auto& f:a.frames)
            if(f.size()!=64000) throw std::runtime_error("Invalid original presentation canvas");
    };
    validate(resources_.concept,24);validate(resources_.title,18);
    for(const auto& a:resources_.cards) validate(a,1);
    const auto& s=resources_.conceptSprite;
    if(s.width<=0 || s.height<=0 || s.width>320 || s.height>200 ||
       s.anchorX<0 || s.anchorX>320 || s.anchorY<0 || s.anchorY>200 ||
       s.indices.size()!=std::size_t(s.width)*s.height)
        throw std::runtime_error("Invalid CONCEPT sprite");
    enter(Screen::LogoIn);
}
const anm_format::Animation& Presentation::movie() const {
    return screen_==Screen::Title?resources_.title:resources_.concept;
}
void Presentation::enter(Screen screen) {
    screen_=screen;ticks_=0;retraces_=0;frame_=0;
    switch(screen) {
    case Screen::LogoIn:case Screen::LogoOut:
        phase_=Phase::Scale;palette_=resources_.conceptPalette;
        logo(screen==Screen::LogoIn?0:160,true);return;
    case Screen::LogoMovie:case Screen::Title:
        phase_=screen==Screen::Title?Phase::FadeIn:Phase::Movie;
        brightness_=screen==Screen::Title?0:100;
        palette_=movie().palette;canvas_=movie().frames[0];break;
    case Screen::LogoHold:
        phase_=Phase::Hold;logo(160,false);return; // Retain movie palette.
    case Screen::Done:
        canvas_.assign(64000,0);pixels_.assign(64000,255);return;
    default: {
        phase_=Phase::FadeIn;brightness_=0;
        const unsigned card=screen==Screen::Presente?0:1+unsigned(screen)-unsigned(Screen::Who1);
        palette_=resources_.cards[card].palette;canvas_=resources_.cards[card].frames[0];break;
    }
    }
    colors();
}
void Presentation::logo(int t,bool scaled) {
    const auto& s=resources_.conceptSprite;
    const int x=scaled?-106+195*t/160:89-s.anchorX;
    const int y=scaled?-64+118*t/160:54-s.anchorY;
    const int w=scaled?532-399*t/160:s.width,h=scaled?328-246*t/160:s.height;
    const std::int64_t dx=(std::int64_t(s.width)<<16)/w,dy=(std::int64_t(s.height)<<16)/h;
    canvas_.assign(64000,0);
    for(int py=std::max(0,y);py<std::min(200,y+h);++py)
        for(int px=std::max(0,x);px<std::min(320,x+w);++px) {
            const auto c=s.indices[std::size_t((py-y)*dy>>16)*s.width+((px-x)*dx>>16)];
            if(c) canvas_[py*320+px]=c;
        }
    brightness_=scaled?std::clamp(100*t/160,0,100):100;
    colors();
}
void Presentation::colors() {
    std::array<std::uint32_t,256> colors{};
    for(unsigned i=0;i<256;++i) {
        std::uint32_t rgb=0;
        for(int shift:{24,16,8}) {
            const unsigned dac=(((palette_[i]>>shift)&255)*63+127)/255;
            rgb=(rgb<<8)|(((dac*brightness_/100)*255+31)/63);
        }
        colors[i]=(rgb<<8)|255;
    }
    for(unsigned i=0;i<64000;++i) pixels_[i]=colors[canvas_[i]];
}
void Presentation::advance(double seconds,bool key) {
    if(screen_==Screen::Done || !std::isfinite(seconds) || seconds<0) return;
    pendingKey_|=key;
    constexpr double pitRate=extremis::pit::rate;
    pitFraction_+=std::fmod(seconds,65536.0/pitRate)*pitRate;
    const double whole=std::floor(pitFraction_);pitFraction_-=whole;
    ticks_=std::uint16_t(ticks_+std::uint16_t(std::fmod(whole,65536.0)));
    retraceFraction_+=std::min(seconds,1.0)*70;
    const bool retrace=retraceFraction_>=1;
    if(retrace) retraceFraction_=std::fmod(retraceFraction_,1.0);
    // Key polling in 0x676a is only in the hold; no global Escape skip.
    if(phase_==Phase::Hold && screen_!=Screen::LogoHold) {
        if(pendingKey_ || ticks_>800) {
            pendingKey_=false;phase_=Phase::FadeOut;brightness_=100;colors();
        }
        return;
    }
    if(!retrace) return;
    if(phase_==Phase::Scale) {
        const int t=std::min<unsigned>(ticks_,160);
        logo(screen_==Screen::LogoIn?t:160-t,true);
        if(ticks_>=160) {
            if(screen_==Screen::LogoIn) enter(Screen::LogoMovie);
            else enter(Screen::Presente); // Both endpoints are black.
        }
    } else if(phase_==Phase::FadeIn) {
        if(brightness_<100) {brightness_+=4;colors();}
        else {
            phase_=screen_==Screen::Title?Phase::Movie:Phase::Hold;
            // ANM's first frame shares the reveal counter; cards reset after fade.
            if(phase_==Phase::Hold) ticks_=0;
        }
    } else if(phase_==Phase::FadeOut) {
        if(brightness_>0) {brightness_-=4;colors();}
        else {
            if(screen_==Screen::Presente) {pass_=1;enter(Screen::Title);}
            else enter(static_cast<Screen>(unsigned(screen_)+1));
        }
    } else if(screen_==Screen::LogoHold) {
        if(++retraces_==70) {
            if(pass_==1) {++pass_;enter(Screen::LogoMovie);}
            else enter(Screen::LogoOut);
        }
    } else if(phase_==Phase::Movie) {
        if(ticks_<=(screen_==Screen::Title?8u:6u)) return;
        ticks_=0;
        if(frame_+1<movie().frames.size()) {canvas_=movie().frames[++frame_];colors();}
        else if(screen_==Screen::LogoMovie) enter(Screen::LogoHold);
        else if(pass_==1) {++pass_;enter(Screen::Title);}
        else enter(Screen::Who1);
    }
}
}

namespace original_presentation {
Screen Presentation::screen() const
{
    return screen_;
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

unsigned Presentation::brightness() const
{
    return brightness_;
}

const std::vector<std::uint8_t>& Presentation::indices() const
{
    return canvas_;
}

const std::vector<std::uint32_t>& Presentation::pixels() const
{
    return pixels_;
}
} // namespace original_presentation
