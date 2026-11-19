#include "extremis/world/pit_clock.h"
#include "extremis/cinematics/player_death.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>

namespace player {
namespace {
int glyph(unsigned char c)
{
    if(c>='0' && c<='9') return c-'0';
    if(c>='A' && c<='Z') return c-'A'+10;
    if(c>='a' && c<='z') return c-'a'+36;
    const std::string punctuation=":-?!',.";
    const auto p=punctuation.find(c);
    return p==std::string::npos ? -1 : int(p)+62;
}
int advance(const std::vector<spr_format::Image>& font,unsigned char c)
{
    const int g=glyph(c); return g<0 ? 4 : font.at(g).width+1;
}
void blit(std::vector<std::uint8_t>& pixels,const spr_format::Image& image,int x,int y,int top,int bottom)
{
    x-=image.anchorX; y-=image.anchorY;
    for(int row=0;row<image.height;++row) for(int col=0;col<image.width;++col) {
        const int px=x+col,py=y+row;
        const auto index=image.indices[row*image.width+col];
        if(index && px>=0 && px<320 && py>=top && py<bottom) pixels[py*320+px]=index;
    }
}
// Palette arithmetic is in six-bit DAC units, with separate truncation of each
// contribution, as in 0x14611. RGBA is only the presentation representation.
std::uint32_t color(std::uint32_t rgba,unsigned amount,unsigned target,bool brightness=false)
{
    std::uint32_t result=0;
    for(int shift:{24,16,8}) {
        const unsigned v=(((rgba>>shift)&255)*63+127)/255;
        const unsigned c=brightness ? v*amount/100 :
            (v*((100-amount)*65536/100))/65536+target*amount/100;
        result=(result<<8)|((c*255+31)/63);
    }
    return (result<<8)|255;
}
}
DeathResources loadDeathResources(const std::filesystem::path& directory)
{
    DeathResources r;
    r.health=anm_format::loadVga(directory/"MORT0.VGA");
    r.body=spr_format::load(directory/"MORT0.SPR");
    r.oxygen=anm_format::load(directory/"MORT1.ANM");
    r.gameOver=anm_format::loadVga(directory/"GAMEOVER.VGA");
    r.font=spr_format::load(directory/"TYPOOVER.SPR");
    r.menu=anm_format::loadVga(directory/"MENU_F.VGA");
    r.menuFont=spr_format::load(directory/"CON_VUET.SPR");
    r.menuArt=spr_format::load(directory/"MENU2.SPR");
    // Keep original localized prose external. Bounded read from the audited EXE.
    std::ifstream exe(directory.parent_path()/"EXTREMIS.EXE",std::ios::binary);
    exe.seekg(0x21b96);
    if(exe.get()!=8) throw std::runtime_error("Unsupported original game-over text table");
    for(unsigned i=0;i<8;++i) {
        std::string line; char c;
        while(exe.get(c) && c) {
            if(line.size()>=100) throw std::runtime_error("Invalid game-over text");
            line+=c;
        }
        if(!exe) throw std::runtime_error("Missing original game-over text");
        r.lines.push_back(line);
    }
    for(auto table:{0x2369e,0x236be}) for(unsigned i=0;i<3;++i) {
        exe.seekg(table+4*i);
        unsigned pointer=0;
        for(unsigned byte=0;byte<4;++byte) {
            const int value=exe.get();
            if(value<0) throw std::runtime_error("Missing menu text pointer");
            pointer|=unsigned(value)<<(byte*8);
        }
        if(pointer<0x23400 || pointer>0x23500) throw std::runtime_error("Unsupported menu text table");
        exe.seekg(pointer+512);
        std::string line; char c;
        while(exe.get(c) && c) {
            if(line.size()>=100) throw std::runtime_error("Invalid menu text");
            line+=c;
        }
        if(!exe) throw std::runtime_error("Missing menu text");
        r.menuLines.push_back(line);
    }
    return r;
}
DeathPresentation::DeathPresentation(DeathResources resources):resources_(std::move(resources))
{
    for(const auto* movie:{&resources_.health,&resources_.oxygen,&resources_.gameOver,&resources_.menu})
        if(movie->frames.empty() || std::any_of(movie->frames.begin(),movie->frames.end(),
            [](const auto& f){return f.size()!=64000;})) throw std::runtime_error("Invalid death framebuffer");
    if(resources_.body.size()!=6 || resources_.font.size()<69 || resources_.lines.size()!=8 ||
       resources_.menuFont.size()<69 || resources_.menuArt.size()!=3 || resources_.menuLines.size()!=6 ||
       resources_.menuArt[2].width!=62 || resources_.menuArt[2].height!=43)
        throw std::runtime_error("Invalid death sprite/text resources");
    for(const auto* bank:{&resources_.body,&resources_.font,&resources_.menuFont,&resources_.menuArt}) for(const auto& s:*bank)
        if(s.width<=0 || s.height<=0 || s.width>320 || s.height>200 ||
           s.indices.size()!=std::size_t(s.width)*s.height) throw std::runtime_error("Invalid death sprite");
}
void DeathPresentation::loadExplosion(const std::filesystem::path& directory)
{
    if(!resources_.explosion.frames.empty()) return;
    auto movie=anm_format::load(directory/"VAIS_EXP.ANM");
    if(movie.frames.size()!=60 || std::any_of(movie.frames.begin(),movie.frames.end(),
        [](const auto& f){return f.size()!=64000;})) throw std::runtime_error("Invalid ship explosion movie");
    resources_.explosion=std::move(movie);
}
void DeathPresentation::reset()
{
    stage_=Stage::Inactive; cause_=DeathCause::None; step_=frame_=characters_=0;
    ticks_=0; soundStart_=soundStop_=released_=false; pixels_.clear(); background_.clear();
    selection_=0; runMenu_=false;
}
void DeathPresentation::beginMenu()
{
    reset(); enter(Stage::MenuReveal); compose();
}
void DeathPresentation::enter(Stage stage) { stage_=stage; step_=0; ticks_=0; }
void DeathPresentation::begin(DeathCause cause,const std::vector<std::uint32_t>& background)
{
    if(active() || cause==DeathCause::None) return;
    if(background.size()!=64000) throw std::runtime_error("Death needs the original 320x200 canvas");
    if(cause==DeathCause::SelfDestruction && (resources_.explosion.frames.empty() ||
       std::any_of(resources_.explosion.frames.begin(),resources_.explosion.frames.end(),
           [](const auto& f){return f.size()!=64000;}))) throw std::runtime_error("Missing ship explosion movie");
    reset(); cause_=cause; background_=background; enter(Stage::Tint); compose();
}
bool DeathPresentation::takeSoundStart() { const bool b=soundStart_; soundStart_=false; return b; }
bool DeathPresentation::takeSoundStop() { const bool b=soundStop_; soundStop_=false; return b; }
void DeathPresentation::advance(double seconds,bool keyPressed,bool keyHeld)
{
    if(!active() || !std::isfinite(seconds) || seconds<0) return;
    ticks_+=seconds*(extremis::pit::rate);
    switch(stage_) {
    case Stage::Tint:
        if(++step_==26) { soundStart_=true; enter(cause_==DeathCause::Health?Stage::Body:Stage::MovieReveal); }
        break;
    case Stage::Body:
        // 175 draws, y=26..-148 inclusive; no native PIT/retrace wait here.
        if(++step_==175) enter(Stage::Fade);
        break;
    case Stage::MovieReveal:
        if(++step_==26) { stage_=Stage::Movie; step_=0; frame_=0; }
        break;
    case Stage::Movie:
        // Native comparisons: oxygen >25, ship >11; reset counter, no catch-up.
        if(ticks_>=(cause_==DeathCause::SelfDestruction?12:26)) {
            ticks_=0;
            const auto& movie=cause_==DeathCause::SelfDestruction?resources_.explosion:resources_.oxygen;
            if(++frame_==movie.frames.size()) { --frame_; enter(Stage::Hold); }
        }
        break;
    case Stage::Hold:
        if(ticks_>=161) {
            if(cause_==DeathCause::SelfDestruction) soundStop_=true;
            enter(Stage::Fade);
        }
        break;
    case Stage::Fade:
        if(++step_==(cause_==DeathCause::Health?21u:51u)) {
            if(cause_!=DeathCause::SelfDestruction) soundStop_=true;
            enter(Stage::GameOverReveal);
        }
        break;
    case Stage::GameOverReveal:
        if(++step_==26) { enter(Stage::Text); characters_=1; }
        break;
    case Stage::Text: {
        unsigned total=0; for(const auto& line:resources_.lines) total+=line.size();
        if(ticks_>=9) {
            ticks_=0;
            if(characters_<total) ++characters_;
            else { enter(Stage::Wait); released_=!keyHeld; }
        }
        break;
    }
    case Stage::Wait:
        if(!released_) { if(!keyHeld) { released_=true; ticks_=0; } }
        else if(keyPressed || ticks_>=1601) enter(Stage::GameOverFade);
        break;
    case Stage::GameOverFade: if(++step_==101) enter(Stage::Finished); break;
    case Stage::Finished: enter(Stage::MenuReveal); break;
    case Stage::MenuReveal: if(++step_==51) enter(Stage::Menu); break;
    case Stage::Menu: ++step_; break;
    default: break;
    }
    compose();
}
DeathPresentation::Request DeathPresentation::menuInput(int direction,bool confirm)
{
    if(stage_!=Stage::Menu) return Request::None;
    selection_=unsigned(std::clamp(int(selection_)+direction,0,2));
    if(confirm) {
        if(selection_==1) return Request::Unsupported; // Native options/password systems are separate.
        if(runMenu_) {
            if(selection_==0) return Request::Restart;
            runMenu_=false;
        } else {
            if(selection_==2) return Request::Exit;
            runMenu_=true;
        }
        selection_=0; step_=0;
    }
    compose();
    return Request::None;
}
void DeathPresentation::compose()
{
    if(stage_==Stage::MenuReveal || stage_==Stage::Menu) {
        auto indices=resources_.menu.frames[0];
        const auto& art=resources_.menuArt[2];
        for(unsigned y=0;y<43;++y) for(unsigned x=0;x<62;++x) {
            const auto rotation=stage_==Stage::MenuReveal?2:2*(step_+1);
            const auto index=art.indices[y*62+(x+rotation)%62];
            if(index) indices[(y+9)*320+x+251]=index;
        }
        for(unsigned row=0;row<3;++row) {
            if(stage_==Stage::Menu && (step_&7)>=4 && selection_==row) continue;
            const auto& line=resources_.menuLines[row+(runMenu_?3:0)];
            int width=0; for(unsigned char c:line) width+=::player::advance(resources_.menuFont,c);
            int x=39+(149-width)/2;
            for(unsigned char c:line) {
                const int g=glyph(c);
                if(g>=0) blit(indices,resources_.menuFont[g],x,84+40*row,0,200);
                x+=::player::advance(resources_.menuFont,c);
            }
        }
        pixels_.resize(64000);
        for(unsigned i=0;i<64000;++i) pixels_[i]=stage_==Stage::MenuReveal ?
            color(resources_.menu.palette[indices[i]],step_*2,0,true):resources_.menu.palette[indices[i]];
        return;
    }
    if(stage_==Stage::Tint) {
        pixels_=background_;
        const unsigned amount=cause_==DeathCause::Oxygen?75+step_:step_*4;
        for(auto& p:pixels_) {
            if(cause_!=DeathCause::Health) p=color(p,amount,63);
            else {
                const auto r=color(p,amount,63), gb=color(p,amount,0);
                p=(r&0xff000000)|(gb&0x00ffffff);
            }
        }
        return;
    }
    const bool end=stage_>=Stage::GameOverReveal;
    const auto& resource=end?resources_.gameOver:cause_==DeathCause::Health?resources_.health:
        cause_==DeathCause::SelfDestruction?resources_.explosion:resources_.oxygen;
    auto indices=resource.frames[end || cause_==DeathCause::Health?0:frame_];
    if(stage_==Stage::Body) {
        constexpr int x[]{55,75,95,95,90,55};
        // Native transfer rectangle is x55..264, y26..173. Pixels outside it
        // remain the background even though the sprite clip extends to row199.
        auto work=indices;
        blit(work,resources_.body[step_%6],x[step_%6],26-int(step_),26,200);
        for(int y=26;y<=173;++y) std::copy_n(work.begin()+y*320+55,210,indices.begin()+y*320+55);
    }
    if(end && stage_!=Stage::GameOverReveal) {
        unsigned remaining=characters_;
        int y=60;
        for(const auto& line:resources_.lines) {
            int width=0; for(unsigned char c:line) width+=::player::advance(resources_.font,c);
            int x=std::max(0,320-width)/2;
            for(unsigned char c:line) {
                if(!remaining) break;
                --remaining; const int g=glyph(c);
                if(g>=0) blit(indices,resources_.font[g],x,y,0,200);
                x+=::player::advance(resources_.font,c);
            }
            y+=10;
        }
    }
    pixels_.resize(64000);
    for(unsigned i=0;i<64000;++i) {
        auto p=resource.palette[indices[i]];
        if(stage_==Stage::Body && step_<=25) {
            const auto r=color(p,100-step_*4,63),gb=color(p,100-step_*4,0);
            p=(r&0xff000000)|(gb&0x00ffffff);
        } else if(stage_==Stage::MovieReveal) p=color(p,100-step_*4,63);
        else if(stage_==Stage::Fade) p=color(p,100-step_*(cause_==DeathCause::Health?5:2),0,true);
        else if(stage_==Stage::GameOverReveal) p=color(p,step_*4,0,true);
        else if(stage_==Stage::GameOverFade) p=color(p,100-step_,0,true);
        else if(stage_==Stage::Finished) p=255;
        pixels_[i]=p;
    }
}
}

namespace player {
DeathCause DeathPresentation::cause() const
{
    return cause_;
}

DeathPresentation::Stage DeathPresentation::stage() const
{
    return stage_;
}

bool DeathPresentation::active() const
{
    return stage_!=Stage::Inactive;
}

const std::vector<std::uint32_t>& DeathPresentation::pixels() const
{
    return pixels_;
}
} // namespace player
