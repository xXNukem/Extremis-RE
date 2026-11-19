#include "extremis/world/pit_clock.h"
#include "extremis/cinematics/original_intro.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <utility>

namespace original_intro {
namespace {
template<class E> constexpr unsigned index(E e) { return static_cast<unsigned>(e); }
constexpr const char* visualFiles[]{"PSEQ1.ANM","PSEQ2_1.VGA","PSEQ2_2.VGA","PSEQ3.ANM",
    "PSEQ4.ANM","PSEQ6_1.ANM","PSEQ7_1.VGA","PSEQ8.ANM","PSEQ10_1.VGA",
    "PSEQ11_1.VGA","PSEQ12_1.VGA","PSEQ14_1.VGA","PSEQ14_2.VGA"};
constexpr const char* spriteFiles[]{"PSEQ2_3.SPR","PSEQ6_2.SPR","PSEQ7_2.SPR","PSEQ10_2.SPR",
    "PSEQ10_3.SPR","PSEQ11_2.SPR","PSEQ12_2.SPR","PSEQ14_3.SPR","CON_VUET.SPR"};
constexpr unsigned spriteCounts[]{37,4,7,7,2,1,9,5,76};
constexpr const char* soundFiles[]{"","BARGE2.DSS","CABINE.DSS","RADAR.DSS","LABO.DSS",
    "ALARME1.DSS","ALARME4.DSS","TOUCHE.DSS","OXYGENE.DSS","EXPLO.DSS"};
struct Motion { int duration,x0,y0,x1,y1,sprite,panel,blink; };
// EXE file 0x20d9f: thirteen records, not inferred animation keyframes.
constexpr Motion motions[]{
    {40,297,112,189,83,1,-1,-1},{40,187,83,187,83,2,1,-1},
    {40,189,83,166,54,1,-1,-1},{40,164,54,164,54,2,2,-1},
    {40,166,54,166,87,1,-1,-1},{40,164,87,164,87,2,3,-1},
    {40,166,87,189,150,1,-1,-1},{40,187,150,187,150,2,4,-1},
    {40,189,150,189,150,1,-1,-1},{40,187,150,187,150,2,4,-1},
    {40,189,150,166,150,1,-1,-1},{40,164,150,164,150,2,5,1},
    {160,166,150,400,199,1,-1,1}};
Visual visual(Scene s) {
    switch(s) {
    case Scene::Ship:return Visual::Ship;
    case Scene::Cockpit:case Scene::CockpitAlarm:return Visual::Cockpit;
    case Scene::Radar:return Visual::Radar;
    case Scene::Labo:return Visual::Labo;
    case Scene::Message:return Visual::Message;
    case Scene::Stars:return Visual::Stars;
    case Scene::Alarm:return Visual::Alarm;
    case Scene::Panel:return Visual::Panel;
    case Scene::Depart:return Visual::Depart;
    case Scene::Transfer:return Visual::Transfer;
    default:return Visual::Explosion;
    }
}
bool movie(Scene s) {
    return s==Scene::Ship || s==Scene::Radar || s==Scene::Labo || s==Scene::Message || s==Scene::Alarm;
}
unsigned interval(Scene s) {
    return s==Scene::Radar?8:s==Scene::Labo?10:s==Scene::Alarm?20:15;
}
int glyph(unsigned char c) {
    if(c>='0' && c<='9') return c-'0'+1;
    if(c>='A' && c<='Z') return c-'A'+11;
    if(c>='a' && c<='z') return c-'a'+37;
    const std::string punctuation=":-?!',.\x85\x82\x8a\x88\x93\x96\x97";
    const auto p=punctuation.find(char(c));
    return p==std::string::npos?0:int(p)+63;
}
}
const char* soundFile(Sound sound) { return soundFiles[index(sound)]; }
Resources loadResources(const std::filesystem::path& directory) {
    Resources r;
    for(unsigned i=0;i<r.visuals.size();++i) {
        const auto p=directory/visualFiles[i];
        r.visuals[i]=p.extension()==".ANM"?anm_format::load(p):anm_format::loadVga(p);
    }
    for(unsigned i=0;i<r.sprites.size();++i) r.sprites[i]=spr_format::load(directory/spriteFiles[i]);
    // Bounded reads of the original localized prose; never embed it in the port.
    std::ifstream exe(directory.parent_path()/"EXTREMIS.EXE",std::ios::binary);
    constexpr unsigned offsets[]{0x23906,0x23a68}, lengths[]{350,316};
    for(unsigned i=0;i<2;++i) {
        exe.seekg(offsets[i]);
        r.text[i].resize(lengths[i]);
        if(!exe.read(r.text[i].data(),lengths[i]) || exe.get()!=0 ||
           r.text[i].find('\0')!=std::string::npos)
            throw std::runtime_error("Missing or unsupported original introduction text in EXTREMIS.EXE");
    }
    return r;
}
Presentation::Presentation(Resources resources,bool skipAtEntry):resources_(std::move(resources)) {
    for(const auto& a:resources_.visuals) {
        if(a.frames.empty() || a.frames.size()>1000) throw std::runtime_error("Invalid intro animation count");
        for(const auto& f:a.frames) if(f.size()!=64000) throw std::runtime_error("Invalid intro canvas");
    }
    for(unsigned i=0;i<resources_.sprites.size();++i) {
        const auto& bank=resources_.sprites[i];
        if(bank.size()!=spriteCounts[i]) throw std::runtime_error("Unsupported intro sprite count");
        for(const auto& s:bank)
            if(s.width<=0 || s.height<=0 || s.width>320 || s.height>200 ||
               s.anchorX<0 || s.anchorX>320 || s.anchorY<0 || s.anchorY>200 ||
               s.indices.size()!=std::size_t(s.width)*s.height)
                throw std::runtime_error("Invalid intro sprite bounds");
    }
    for(const auto& t:resources_.text)
        if(t.empty() || t.size()>1024 || t.size()%2 || t.find('\0')!=std::string::npos)
            throw std::runtime_error("Invalid intro text bounds");
    if(skipAtEntry) { scene_=Scene::Done;canvas_.assign(64000,0);pixels_.assign(64000,255); }
    else enter(Scene::Ship);
}
void Presentation::cue(unsigned voice,Sound sound,unsigned volume) { cues_.push_back({voice,sound,volume}); }
std::vector<Cue> Presentation::takeCues() { auto result=std::move(cues_); cues_.clear(); return result; }
void Presentation::finish() {
    cue(1); cue(0); scene_=Scene::Done; canvas_.assign(64000,0); pixels_.assign(64000,255);
}
void Presentation::enter(Scene s) {
    scene_=s; frame_=record_=time_=phase_=wait_=0; brightness_=100;
    revealing_=s==Scene::Ship; messageSprites_=false;
    if(s==Scene::Done) { finish(); return; }
    if(s==Scene::Fade || s==Scene::AudioWait) return;
    if(s!=Scene::Radar) { cue(1); cue(0); }
    else cue(0);
    switch(s) {
    case Scene::Ship:brightness_=0; cue(0,Sound::Barge,32);break;
    case Scene::Cockpit:cue(0,Sound::Cabin,48);break;
    case Scene::Radar:textPosition_=0;moviePass_=1;cue(0,Sound::Cabin,48);cue(1,Sound::Radar,48);break;
    case Scene::Labo:cue(0,Sound::Labo,48);break;
    case Scene::CockpitAlarm:cue(0,Sound::Alarm1,48);break;
    case Scene::Message:textPosition_=0;cue(0,Sound::Alarm4,48);break;
    case Scene::Stars: {
        cue(0,Sound::Alarm1,16);
        // Adapted deterministic RNG. DOS RNG/state has not been recovered.
        std::uint32_t state=0x1993;
        const auto random=[&state]() { state^=state<<13;state^=state>>17;state^=state<<5;return state; };
        for(unsigned i=0;i<stars_.size();++i) {
            const unsigned count=400-i;
            const int x=int(random()&2047)*(count&1?-1:1),z=1000+int(random()&2047);
            const int y=int(random()&1023)*(count&2?-1:1);
            stars_[i]={x,z,y};
        }
        break;
    }
    case Scene::Alarm:case Scene::Panel:cue(0,Sound::Alarm4,48);break;
    case Scene::Depart:case Scene::Transfer:case Scene::Explosion:
        cue(0,Sound::Alarm1,16);cue(1,Sound::Oxygen,48);break;
    default:break;
    }
    palette_=resources_.visuals[index(visual(s))].palette; palette_[0]=255;
    if(movie(s)) movieFrame(); else compose();
}
void Presentation::sprite(Bank bank,int number,int x,int y,int w,int h,bool anchors) {
    const auto& b=resources_.sprites[index(bank)];
    // Original overflow/out-of-range requests must never address host memory.
    if(number<1 || unsigned(number)>b.size()) return;
    const auto& s=b[number-1];
    const bool scaled=w!=0 || h!=0;
    if(!scaled) { w=s.width;h=s.height;if(anchors){x-=s.anchorX;y-=s.anchorY;} }
    if(w<=0 || h<=0 || w>65535 || h>65535) return;
    const std::int64_t dx=(std::int64_t(s.width)<<16)/w,dy=(std::int64_t(s.height)<<16)/h;
    for(int py=std::max(0,y);py<std::min(200,y+h);++py)
        for(int px=std::max(0,x);px<std::min(320,x+w);++px) {
            const int sx=int((px-x)*dx>>16),sy=int((py-y)*dy>>16);
            if(sx>=s.width || sy>=s.height) continue;
            const auto c=s.indices[std::size_t(sy)*s.width+sx];
            if(c) canvas_[py*320+px]=c;
        }
}
void Presentation::drawText(unsigned which) {
    int x=10,y=10;
    const auto& text=resources_.text[which];
    for(unsigned i=0;i<std::min<unsigned>(textPosition_,text.size());++i) {
        const unsigned char c=text[i];
        if(c=='\n') continue;
        if(c=='\r') {x=10;y+=11;continue;}
        const int n=glyph(c);
        if(n) sprite(Bank::Font,n,x,y);
        x+=n?resources_.sprites[index(Bank::Font)][n-1].width:4;
    }
    textPosition_=std::min<unsigned>(textPosition_+2,text.size());
}
void Presentation::movieFrame() {
    canvas_=resources_.visuals[index(visual(scene_))].frames.at(frame_);
    if(scene_==Scene::Radar || scene_==Scene::Message) drawText(scene_==Scene::Radar?0:1);
    colors();
}
void Presentation::colors() {
    std::array<std::uint32_t,256> palette{};
    for(unsigned i=0;i<256;++i) {
        if(brightness_==100) {palette[i]=palette_[i];continue;}
        std::uint32_t c=0;
        for(int shift:{24,16,8}) {
            const unsigned dac=(((palette_[i]>>shift)&255)*63+127)/255;
            c=(c<<8)|(((dac*brightness_/100)*255+31)/63);
        }
        palette[i]=(c<<8)|255;
    }
    for(unsigned i=0;i<64000;++i) pixels_[i]=palette[canvas_[i]];
}
void Presentation::compose() {
    auto background=visual(scene_);
    const int t=time_,p=phase_;
    if(scene_==Scene::CockpitAlarm && t%40>=20) background=Visual::CockpitFlash;
    if(scene_==Scene::Explosion && (t>=400 || (t>=320 && (t/10)&1))) background=Visual::ExplosionFlash;
    canvas_=resources_.visuals[index(background)].frames[0];
    switch(scene_) {
    case Scene::Cockpit:case Scene::CockpitAlarm: {
        const int q=p>>3;
        sprite(Bank::Cockpit,17+(q<15?q:29-q),129,70);
        constexpr int ys[]{73,80,87,94,101}, masks[]{512,256,128,64,32};
        for(unsigned i=0;i<5;++i) sprite(Bank::Cockpit,t&masks[i]?33:32,192,ys[i]);
        sprite(Bank::Cockpit,1+t/20,136,74);
        const int r=t/40;
        sprite(Bank::Cockpit,34+(r<4?r:r<8?(~r)&3:0),91,68);
        break;
    }
    case Scene::Message:
        canvas_.assign(64000,0);sprite(Bank::Message,1+(p&3),169,113);drawText(1);break;
    case Scene::Stars:
        sprite(Bank::Stars,t<160?1:2+(t-160)/20,142,67);
        for(auto& star:stars_) {
            const int x=160+256*star[0]/star[1],y=71+256*star[2]/star[1];
            if(x>=0 && x<320 && y>=0 && y<200 && !canvas_[y*320+x])
                canvas_[y*320+x]=std::clamp((star[1]-1000)/136,1,15);
            star[1]+=10;if(star[1]>=3047) star[1]-=2047;
        }
        break;
    case Scene::Panel: {
        const auto& m=motions[record_];
        if(m.panel!=-1) sprite(Bank::Panel,m.panel,151,45);
        sprite(Bank::Panel,m.blink!=-1 && (t&32)?6:7,56,95);
        sprite(Bank::Hand,m.sprite,m.x0+(m.x1-m.x0)*t/m.duration,
               m.y0+(m.y1-m.y0)*t/m.duration,0,0,true);
        // Called once upon entry to each record (not on every zero-tick pulse).
        break;
    }
    case Scene::Depart:sprite(Bank::Depart,1,72+58*t/320,30+35*t/320,82-53*t/320,103-66*t/320);break;
    case Scene::Transfer:
        // 0x207fa selects stale compressed VGA bytes, not PSEQ12_2.SPR.
        // Safety limitation: retain background/timing/audio, omit invalid draw.
        break;
    case Scene::Explosion: {
        if(t>=320 && !exploded_) {cue(0,Sound::Explosion,64);cue(1);exploded_=true;}
        const int q=p/40;
        sprite(Bank::Explosion,1+(q<=4?q:(~q)&3),118+65*t/480,65-53*t/480,18+75*t/480,18+73*t/480);
        break;
    }
    default:break;
    }
    colors();
}
bool Presentation::unpaced() const {
    return scene_==Scene::Cockpit || scene_==Scene::CockpitAlarm || scene_==Scene::Stars ||
           scene_==Scene::Panel || scene_==Scene::Depart || scene_==Scene::Transfer || scene_==Scene::Explosion;
}
void Presentation::advance(double seconds,bool escape,bool voice0Active) {
    if(scene_==Scene::Done || !std::isfinite(seconds) || seconds<0) return;
    pendingEscape_|=escape;
    // Whole PIT ticks, retaining the phase of the interrupt clock across resets.
    constexpr double pitRate=extremis::pit::rate;
    pitFraction_+=std::fmod(seconds,65536.0/pitRate)*pitRate;
    const double whole=std::floor(pitFraction_);pitFraction_-=whole;
    const auto delta=std::uint16_t(std::fmod(whole,65536.0));
    wait_=std::uint16_t(wait_+delta);
    retraceFraction_+=std::min(seconds,1.0)*70.0;
    const bool retrace=retraceFraction_>=1;
    if(retrace) retraceFraction_=std::fmod(retraceFraction_,1.0);
    if(scene_==Scene::AudioWait) { if(!voice0Active) finish();return; }
    if(scene_==Scene::Fade) {
        if(retrace) { if(brightness_==0) enter(Scene::AudioWait);else {brightness_-=4;colors();} }
        return;
    }
    if(revealing_) {
        if(retrace) { if(brightness_==100) revealing_=false;else {brightness_+=4;colors();} }
        return; // Escape remains pending through the uninterruptible reveal.
    }
    const bool recordScene=scene_==Scene::Panel || scene_==Scene::Transfer;
    if(pendingEscape_ && !recordScene && !(scene_==Scene::Message && messageSprites_ && wait_<15)) {
        finish();return;
    }
    if(movie(scene_) && !messageSprites_) {
        if(wait_<=interval(scene_) || !retrace) return;
        wait_=0;
        if(++frame_<resources_.visuals[index(visual(scene_))].frames.size()) {movieFrame();return;}
        if(scene_==Scene::Radar && textPosition_<resources_.text[0].size()) {
            frame_=0;++moviePass_;cue(1,Sound::Radar,48);movieFrame();return;
        }
        if(scene_==Scene::Message) {messageSprites_=true;phase_=0;compose();return;}
        enter(static_cast<Scene>(index(scene_)+1));return;
    }
    if(scene_==Scene::Message) {
        if(wait_<15) return;
        wait_=0;
        if(textPosition_>=resources_.text[1].size()) {enter(Scene::Stars);return;}
        phase_=(phase_+1)&3;compose();return;
    }
    time_=std::uint16_t(time_+wait_);phase_=std::uint16_t(phase_+wait_);wait_=0;
    if(scene_==Scene::Cockpit || scene_==Scene::CockpitAlarm) {
        if(phase_>232) phase_-=232;
        if(time_>320) {enter(static_cast<Scene>(index(scene_)+1));return;}
    } else if(recordScene) {
        const unsigned duration=scene_==Scene::Panel?motions[record_].duration:20;
        if(time_>=duration) {
            if(pendingEscape_) {finish();return;}
            time_=0;
            if(++record_==(scene_==Scene::Panel?13u:9u)) {enter(static_cast<Scene>(index(scene_)+1));return;}
            if(scene_==Scene::Panel && motions[record_].panel!=-1) cue(1,Sound::Touch,48);
        }
    } else if(scene_==Scene::Stars && time_>=280) {enter(Scene::Alarm);return;}
    else if(scene_==Scene::Depart && time_>=320) {enter(Scene::Transfer);return;}
    else if(scene_==Scene::Explosion) {
        if(phase_>=320) phase_-=320;
        if(time_>=480) {enter(Scene::Fade);return;}
    }
    compose();
}
}

namespace original_intro {
Scene Presentation::scene() const
{
    return scene_;
}

unsigned Presentation::frame() const
{
    return frame_;
}

unsigned Presentation::record() const
{
    return record_;
}

unsigned Presentation::textPosition() const
{
    return textPosition_;
}

unsigned Presentation::moviePass() const
{
    return moviePass_;
}

const std::vector<std::uint8_t>& Presentation::indices() const
{
    return canvas_;
}

const std::vector<std::uint32_t>& Presentation::pixels() const
{
    return pixels_;
}
} // namespace original_intro
