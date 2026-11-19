#include "extremis/cinematics/original_elevator.h"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace original_elevator {
Resources loadResources(const std::filesystem::path& directory) {
    auto background=anm_format::loadVga(directory/"A_LEVEL.VGA");
    auto sprites=spr_format::load(directory/"A_LEVEL.SPR");
    if(sprites.size()!=1) throw std::runtime_error("A_LEVEL.SPR must contain one image");
    return {std::move(background),std::move(sprites[0])};
}
Rectangle rectangle(int q) { return {81+19*q/200,146-140*q/200,69+28*q/200,74+31*q/200}; }
Presentation::Presentation(Resources resources,int delta):resources_(std::move(resources)) {
    const auto& s=resources_.sprite;
    if(resources_.background.frames.size()!=1 || resources_.background.frames[0].size()!=64000 ||
       s.width!=97 || s.height!=105 || s.indices.size()!=97*105 || s.anchorX || s.anchorY)
        throw std::runtime_error("Invalid A_LEVEL resources");
    descending_=delta<0;
    start_=descending_?(delta==-1?260:460):0;
    end_=descending_?0:(delta==1?260:460);
    q_=std::int16_t(start_);pixels_.resize(64000);draw(q_);colors();
}
void Presentation::draw(int q) {
    drawnQ_=q;canvas_=resources_.background.frames[0];
    const auto r=rectangle(q); const auto& s=resources_.sprite;
    // Same 16.16 nearest-source arithmetic as the established DOS scaler.
    const int dx=(s.width<<16)/r.w,dy=(s.height<<16)/r.h;
    for(int y=std::max(0,r.y);y<std::min(200,r.y+r.h);++y)
        for(int x=std::max(0,r.x);x<std::min(320,r.x+r.w);++x) {
            const auto c=s.indices[((y-r.y)*dy>>16)*s.width+((x-r.x)*dx>>16)];
            if(c) canvas_[y*320+x]=c;
        }
}
void Presentation::colors() {
    std::array<std::uint32_t,256> palette{};
    for(unsigned i=0;i<256;++i) {
        std::uint32_t rgb=0;
        for(int shift:{24,16,8}) {
            const unsigned c=(((resources_.background.palette[i]>>shift)&255)*63+127)/255;
            const unsigned dac=c*unsigned(brightness_)/100;
            rgb=(rgb<<8)|((dac*255+31)/63);
        }
        palette[i]=(rgb<<8)|255;
    }
    for(unsigned i=0;i<64000;++i) pixels_[i]=palette[canvas_[i]];
}
void Presentation::step() {
    switch(phase_) {
    case Phase::Initial: phase_=Phase::FadeIn; break; // Initial draw's retrace.
    case Phase::FadeIn:
        brightness_=fade_;fade_+=2;
        if(fade_>100) { phase_=Phase::Move;motor_=true; }
        break;
    case Phase::Move: {
        draw(q_);
        const int step=std::uint16_t(speed_)>>2;
        if(descending_) {
            speed_=std::int16_t(q_<start_/2+8?std::max(1,int(speed_)-1):int(speed_)+1);
            q_=std::int16_t(q_-step);
            if(q_<=end_) phase_=Phase::Hold;
        } else {
            speed_=std::int16_t(speed_+(q_>end_/2-10?-1:1));
            q_=std::int16_t(q_+step);
            if(q_>end_) phase_=Phase::Hold;
        }
        break;
    }
    case Phase::Hold:
        if(++hold_==30) {motor_=false;phase_=Phase::FadeOut;fade_=100;}
        break;
    case Phase::FadeOut:
        brightness_=fade_;fade_-=2;
        if(fade_<0) phase_=Phase::Done;
        break;
    case Phase::Done:return;
    }
    colors();
}
}

namespace original_elevator {
Phase Presentation::phase() const
{
    return phase_;
}

int Presentation::q() const
{
    return drawnQ_;
}

int Presentation::brightness() const
{
    return brightness_;
}

bool Presentation::motor() const
{
    return motor_;
}

const std::vector<std::uint8_t>& Presentation::indices() const
{
    return canvas_;
}

const std::vector<std::uint32_t>& Presentation::pixels() const
{
    return pixels_;
}
} // namespace original_elevator
