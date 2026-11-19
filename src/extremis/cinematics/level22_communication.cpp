#include "extremis/world/pit_clock.h"
#include "extremis/cinematics/level22_communication.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace communication {
namespace {
int glyph(unsigned char c) {
    if(c>='0' && c<='9') return c-'0';
    if(c>='A' && c<='Z') return c-'A'+10;
    if(c>='a' && c<='z') return c-'a'+36;
    const std::string punctuation=":-?!',.\x85\x82\x8a\x88\x93\x96\x97";
    const auto p=punctuation.find(char(c));return p==std::string::npos?-1:int(p)+62;
}
void image(std::vector<std::uint8_t>& canvas,const spr_format::Image& s,int x,int y,bool opaque=false) {
    for(int row=0;row<s.height;++row) for(int col=0;col<s.width;++col) {
        const int xx=x+col,yy=y+row;const auto c=s.indices[row*s.width+col];
        if((c || opaque) && xx>=0 && xx<320 && yy>=0 && yy<200) canvas[yy*320+xx]=c;
    }
}
void clearText(std::vector<std::uint8_t>& canvas) {
    for(int y=19;y<=150;++y) std::fill_n(canvas.begin()+y*320+96,160,0);
}
int phaseWord(const game_lab::Level& level,unsigned sector) {
    if(level.zoneIndex!=21 || sector!=16 || level.connectionStates.size()<=44) return 0;
    const auto word=level.connectionStates[44];
    return (word>>8)?static_cast<std::int8_t>(word&255):0;
}
}
unsigned doorFade(const game_lab::Level& level,unsigned sector) {
    const int phase=phaseWord(level,sector);return phase>0 && phase<=60?unsigned(phase)*100/60:0;
}
bool trigger(game_lab::Level& level,unsigned sector) {
    if(phaseWord(level,sector)<=60) return false;
    level.connectionStates[44]=0;return true;
}
Resources loadResources(const std::filesystem::path& directory) {
    Resources r;
    r.prompt=anm_format::loadVga(directory/"CON_VUE.VGA");
    r.first=anm_format::loadVga(directory/"CON_ECR1.VGA");
    r.second=anm_format::loadVga(directory/"CON_ECR2.VGA");
    r.font=spr_format::load(directory/"CON_VUET.SPR");
    r.sprites=spr_format::load(directory/"CON_VISA.SPR");
    std::ifstream exe(directory.parent_path()/"EXTREMIS.EXE",std::ios::binary);
    const auto text=[&](unsigned offset,unsigned length) {
        std::string value(length,'\0');exe.seekg(offset);
        if(!exe.read(value.data(),length) || exe.get()!=0 || value.find('\0')!=std::string::npos)
            throw std::runtime_error("Missing or unsupported communication text in EXTREMIS.EXE");
        return value;
    };
    r.question={text(0x21e04,34),text(0x21e27,7),text(0x21e2b,3)};
    constexpr unsigned offsets[]{0x21e4d,0x21f3f,0x22216,0x22407,0x22698,0x22942,0x22c76};
    constexpr unsigned lengths[]{241,726,496,656,681,819,250};
    for(unsigned i=0;i<7;++i) r.pages[i]=text(offsets[i],lengths[i]);
    return r;
}
Presentation::Presentation(Resources resources):resources_(std::move(resources)) {
    for(const auto* a:{&resources_.prompt,&resources_.first,&resources_.second})
        if(a->frames.size()!=1 || a->frames[0].size()!=64000) throw std::runtime_error("Invalid communication canvas");
    if(resources_.font.size()!=76 || resources_.sprites.size()!=78) throw std::runtime_error("Invalid communication sprites");
    for(const auto* bank:{&resources_.font,&resources_.sprites}) for(const auto& s:*bank)
        if(s.width<=0 || s.height<=0 || s.width>320 || s.height>200 ||
           s.indices.size()!=std::size_t(s.width*s.height)) throw std::runtime_error("Invalid communication sprite bounds");
    for(const auto& s:resources_.pages) if(s.empty() || s.size()>1024) throw std::runtime_error("Invalid communication page");
    for(const auto& s:resources_.question) if(s.empty() || s.size()>128) throw std::runtime_error("Invalid communication question");
    screens_={resources_.first.frames[0],resources_.second.frames[0]};render();
}
void Presentation::enter(Phase phase) { phase_=phase;wait_=0;pulse_=0; }
void Presentation::beginPage(bool escape) {
    for(auto& screen:screens_) clearText(screen);
    characters_=0;
    if(escape) {frame_=73;enter(Phase::Closing);}
    else {enter(Phase::Typing);animate();characters_=1;}
}
void Presentation::animate() {
    constexpr unsigned portraits[]{73,75,76,75,73,75,76,76,75,76,75,73,74,73,74,76};
    for(auto& screen:screens_) {
        // DOS calls each helper once for each separate background.
        image(screen,resources_.sprites[portraits[portrait_>>3]],46,53,true);
        portrait_=(portrait_+1)&127;
        for(int y=181;y<192;++y) std::fill_n(screen.begin()+y*320+11,291,0);
        image(screen,resources_.sprites[77],scan_,181,true);
        scan_+=scanDelta_;if(scan_==11 || scan_==271) scanDelta_=-scanDelta_;
    }
}
void Presentation::advance(double seconds,Input input) {
    if(phase_==Phase::Done || !std::isfinite(seconds) || seconds<0) return;
    // Existing port convention: nominal PIT and retrace, bounded host service;
    // unpaced acknowledgement effects use 70 Hz, not a claim about DOS CPU speed.
    pitFraction_+=seconds*(extremis::pit::rate);
    const auto ticks=static_cast<unsigned>(std::min(65535.0,std::floor(pitFraction_)));
    pitFraction_-=std::floor(pitFraction_);wait_=std::min(65535u,wait_+ticks);
    if(phase_==Phase::Choice || phase_==Phase::Typing || phase_==Phase::Acknowledge) {
        blink_+=ticks;if(blink_>=160) blink_=0;
    }
    pulse_+=seconds*70;const bool pulse=pulse_>=1;if(pulse) pulse_=0;
    switch(phase_) {
    case Phase::PromptIn:case Phase::ScreenIn:
        if(pulse) {
            if(brightness_<100) brightness_+=2;
            else if(phase_==Phase::PromptIn) {characters_=1;enter(Phase::Question);}
            else {frame_=70;enter(Phase::Opening);}
        } break;
    case Phase::Question:
        if(wait_>8) {wait_=0;if(characters_<resources_.question[0].size()) ++characters_;
            else {blink_=0;enter(Phase::Choice);} } break;
    case Phase::Choice:
        if(input.choice) yes_=input.choice>0;
        if(input.action) enter(Phase::PromptOut);
        break;
    case Phase::PromptOut:case Phase::ScreenOut:
        if(pulse) {
            if(brightness_>=2) brightness_-=2;
            else if(phase_==Phase::PromptOut && yes_) enter(Phase::ScreenIn);
            else enter(Phase::Done);
        } break;
    case Phase::Opening:
        if(wait_>20) {wait_=0;if(frame_<73) ++frame_;else {blink_=0;beginPage(input.escape);}} break;
    case Phase::Typing:
        if(wait_>4) {
            wait_=0;animate();
            if(characters_<resources_.pages[page_].size()) ++characters_;
            else enter(Phase::Acknowledge);
        } break;
    case Phase::Acknowledge:
        if(pulse) animate();
        if(input.action) {
            if(page_==6) {for(auto& s:screens_) clearText(s);frame_=73;enter(Phase::Closing);}
            else {++page_;beginPage(input.escape);}
        } break;
    case Phase::Closing:
        if(wait_>20) {wait_=0;if(frame_>70) --frame_;else {image(screens_[0],resources_.sprites[69],46,53);enter(Phase::ScreenOut);}} break;
    case Phase::Done:break;
    }
    render();
}
void Presentation::render() {
    const bool prompt=phase_<=Phase::PromptOut;
    const bool reading=phase_==Phase::Typing || phase_==Phase::Acknowledge;
    auto canvas=prompt?resources_.prompt.frames[0]:screens_[reading && blink_>=80?1:0];
    const auto label=[&](const std::string& text,int y,unsigned count) {
        int width=0;for(unsigned char c:text) {int g=glyph(c);width+=g>=0?resources_.font[g].width+1:4;}
        int x=std::max(0,(320-width)/2);
        for(unsigned i=0;i<std::min<unsigned>(count,text.size());++i) {
            const int g=glyph(text[i]);if(g>=0) {image(canvas,resources_.font[g],x,y);x+=resources_.font[g].width+1;} else x+=4;
        }
    };
    if(prompt) {
        if(phase_!=Phase::PromptIn) label(resources_.question[0],20,phase_==Phase::Question?characters_:128);
        if(phase_==Phase::Choice || phase_==Phase::PromptOut) {
            if(!yes_ || blink_<80) label(resources_.question[1],30,128);
            if(yes_ || blink_<80) label(resources_.question[2],40,128);
        }
    } else if(phase_==Phase::Typing || phase_==Phase::Acknowledge) {
        clearText(canvas);int x=96,y=19;
        for(unsigned i=0;i<characters_;++i) {
            const unsigned char c=resources_.pages[page_][i];
            if(c=='\r') {x=96;y+=6;continue;}
            const int g=glyph(c);if(g>=0) image(canvas,resources_.sprites[g],x,y);x+=4;
        }
    } else if(phase_==Phase::Opening || phase_==Phase::Closing) {
        canvas=screens_[0];image(canvas,resources_.sprites[frame_-1],46,53);
    }
    const auto& palette=prompt?resources_.prompt.palette:resources_.first.palette;
    for(unsigned i=0;i<64000;++i) {
        const auto c=palette[canvas[i]];const unsigned b=phase_==Phase::Done?0:brightness_;
        pixels_[i]=(((c>>24)*b/100)<<24)|((((c>>16)&255)*b/100)<<16)|((((c>>8)&255)*b/100)<<8)|255;
    }
}
}

namespace communication {
Phase Presentation::phase() const
{
    return phase_;
}

unsigned Presentation::page() const
{
    return page_;
}

unsigned Presentation::characters() const
{
    return characters_;
}

unsigned Presentation::brightness() const
{
    return brightness_;
}

bool Presentation::yes() const
{
    return yes_;
}

const std::vector<std::uint32_t>& Presentation::pixels() const
{
    return pixels_;
}
} // namespace communication
