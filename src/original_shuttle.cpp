#include "original_shuttle.h"
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace original_shuttle {
namespace {
constexpr Stage stages[]{
    {"NAV_INTE.VGA","ALARME4.DSS",1,20},{"NAV_HANG.VGA","ALARME3.DSS",1,26},
    {"NAV_SEQ3.ANM","DEPART.DSS",61,16},{"NAV_SEQ4.ANM","BARGE2.DSS",49,19},
    {"VAIS_EXP.ANM","EXPLO.DSS",60,11},{"NAV_INTE.VGA","CABINE.DSS",1,20},
    {"NAV_SEQ7.ANM","BARGE2.DSS",60,21},{"NAV_SEQ8.ANM","BARGE1.DSS",62,20},
    {"ARR_BASE.ANM","BARGE1.DSS",62,20},{"FSEQ2_1.VGA","VENT.DSS",1,10}};
constexpr int records[9][6]={{40,11,73,16,5,1},{216,10,71,18,18,8},{88,110,22,14,10,26},
    {120,110,22,14,8,36},{152,114,4,6,13,44},{161,114,4,6,13,57},
    {170,114,4,6,13,44},{188,109,3,15,13,70},{203,110,30,14,6,83}};
int glyph(unsigned char c) {
    if(c>='0' && c<='9') return c-'0'+1;
    if(c>='A' && c<='Z') return c-'A'+11;
    if(c>='a' && c<='z') return c-'a'+37;
    const std::string punctuation=":-?!',.\x85\x82\x8a\x88\x93\x96\x97";
    auto p=punctuation.find(char(c));return p==std::string::npos?0:int(p)+63;
}
bool movie(Scene s) { return stages[unsigned(s)].frames>1; }
}
const Stage& stage(Scene s) {return stages[unsigned(s)];}
Resources loadResources(const std::filesystem::path& directory) {
    Resources r;
    for(unsigned i=0;i<10;++i) r.visuals[i]=stages[i].frames>1?
        anm_format::load(directory/stages[i].visual):anm_format::loadVga(directory/stages[i].visual);
    const char* names[]{"NAV_INST.SPR","NAV_INS2.SPR","NAV_SEQ2.SPR","INBASE.SPR"};
    for(unsigned i=0;i<4;++i) r.sprites[i]=spr_format::load(directory/names[i]);
    std::ifstream exe(directory.parent_path()/"EXTREMIS.EXE",std::ios::binary);
    const unsigned offsets[]{0x22e2c,0x22edf,0x22fa5,0x2306b},lengths[]{178,197,197,214};
    for(unsigned i=0;i<4;++i) {
        exe.seekg(offsets[i]);r.text[i].resize(lengths[i]);
        if(!exe.read(r.text[i].data(),lengths[i]) || exe.get()!=0 || r.text[i].find('\0')!=std::string::npos)
            throw std::runtime_error("Missing or unsupported shuttle text in EXTREMIS.EXE");
    }
    return r;
}
Presentation::Presentation(Resources r,bool inherited):resources_(std::move(r)),escape_(inherited) {
    for(unsigned i=0;i<10;++i) {
        if(resources_.visuals[i].frames.size()!=stages[i].frames) throw std::runtime_error("Invalid shuttle frame count");
        for(const auto& f:resources_.visuals[i].frames) if(f.size()!=64000) throw std::runtime_error("Invalid shuttle frame");
    }
    const unsigned counts[]{69,89,16,7};
    for(unsigned i=0;i<4;++i) {
        if(resources_.sprites[i].size()!=counts[i]) throw std::runtime_error("Invalid shuttle sprite count");
        for(const auto& s:resources_.sprites[i]) if(s.width<=0 || s.height<=0 || s.width>320 || s.height>200 ||
            s.anchorX || s.anchorY || s.indices.size()!=std::size_t(s.width*s.height)) throw std::runtime_error("Invalid shuttle sprite");
        if(resources_.text[i].empty() || resources_.text[i].size()>1024) throw std::runtime_error("Invalid shuttle script");
    }
    states_.fill(-1);states_[0]=0x8000;enter(Scene::Startup);
}
std::vector<int> Presentation::takeCues() {auto result=std::move(cues_);cues_.clear();return result;}
void Presentation::sprite(unsigned bank,int n,int x,int y,int w,int h) {
    if(n<1 || unsigned(n)>resources_.sprites[bank].size()) return;
    const auto& s=resources_.sprites[bank][n-1];
    if(w<0) {w=s.width;h=s.height;} if(w<=0 || h<=0) return;
    const std::int64_t dx=(std::int64_t(s.width)<<16)/w,dy=(std::int64_t(s.height)<<16)/h;
    for(int py=std::max(0,y);py<std::min(200,y+h);++py)
        for(int px=std::max(0,x);px<std::min(320,x+w);++px) {
            auto color=s.indices[((py-y)*dy>>16)*s.width+((px-x)*dx>>16)];
            if(color) canvas_[py*320+px]=color;
        }
}
void Presentation::callback() {
    for(unsigned i=0;i<9;++i) {
        auto& state=states_[i];const auto& r=records[i];
        if(state==-1) continue;
        if(state&0x8000) {
            int h=state&0x7fff;sprite(1,i==1?7:r[5],r[0],r[1]+r[3]-h,r[2],2*h);
            if(++h==r[3]) {state=0;if(i<8) states_[i+1]=0x8000;} else state=h|0x8000;
        } else {
            if(i==1) sprite(1,7,r[0],r[1]);
            sprite(1,r[5]+state,r[0]+(i==1?35:0),r[1]+(i==1?2:0));
            state+=tick_;if(state==r[4]) state=0;
        }
    }
    sprite(1,6,121,0);int x=125,y=5;
    const auto& text=resources_.text[script_];
    for(unsigned i=0;i<std::min<unsigned>(characters_,text.size());++i) {
        unsigned char ch=text[i];if(ch=='\r') {x=125;y+=5;continue;}
        if(ch==' ') {x+=4;continue;}
        int n=glyph(ch);if(n>0 && unsigned(n)<=resources_.sprites[0].size()) {
            sprite(0,n,x,y);x+=resources_.sprites[0][n-1].width+1;
        }
    }
    textDone_=characters_>=text.size();if(!textDone_) ++characters_;
}
void Presentation::colors(unsigned brightness) {
    for(unsigned i=0;i<64000;++i) {
        auto c=palette_[canvas_[i]];std::uint32_t out=255;
        for(int shift:{24,16,8}) {
            unsigned v=(c>>shift)&255;
            if(brightness!=100) {v=(v>>2)*brightness/100;v=(v<<2)|(v>>4);}
            out|=v<<shift;
        }
        pixels_[i]=out;
    }
}
void Presentation::enter(Scene s) {
    scene_=s;ticks_=0;frame_=0;waiting_=false;cut_=false;
    if(s==Scene::Done) return;
    cues_.push_back(-1);cues_.push_back(int(s));
    palette_=resources_.visuals[unsigned(s)].palette;palette_[0]=255;
    canvas_=resources_.visuals[unsigned(s)].frames[0];
    if(s==Scene::Startup) {sprite(1,89,0,39);background_=canvas_;colors();return;}
    if(s==Scene::Cruise) {
        script_=2;characters_=1;textDone_=false;tick_=true;total_=0;
        callback();background_=canvas_;
        // Host deterministic RNG adaptation; original seed/state remains uncalibrated.
        std::uint32_t rng=0x22;auto next=[&](){rng=rng*1664525u+1013904223u;return rng>>8;};
        for(unsigned i=0;i<200;++i) {auto& a=stars_[i];a.x=next()&2047;if((200-i)&1) a.x=-a.x;
            a.z=(next()&4095)+1000;a.y=int(next()&1023)-500;}
    } else background_=canvas_;
    cut_=true;std::fill(pixels_.begin(),pixels_.end(),255);
}
void Presentation::draw() {
    if(movie(scene_)) {canvas_=resources_.visuals[unsigned(scene_)].frames[frame_];if(scene_==Scene::Nav3) {tick_=true;callback();}}
    else if(scene_==Scene::Startup) {
        if(textDone_ && script_==0) {script_=1;characters_=1;textDone_=false;}
        canvas_=background_;callback();
    } else if(scene_==Scene::Hangar) {
        canvas_=background_;sprite(2,c_+1,91,145);
        if(hangarStep_>=12 && hangarStep_<24) sprite(2,int(hangarStep_%12)+5,16,21);
        background_=canvas_;
    } else if(scene_==Scene::Cruise) {
        if(textDone_) {characters_=1;if(total_>3200) script_=3;}
        canvas_=background_;callback();background_=canvas_;
        constexpr unsigned char colors[]{255,6,13,17,25,31,43,82,132,159,188,248};
        for(auto& a:stars_) {
            int x=160+256*a.x/a.z,y=71+256*a.y/a.z;
            if(x>=0 && x<320 && y>=39 && y<=103) {
                if(canvas_[y*320+x]==0) canvas_[y*320+x]=colors[std::min(11,a.z/500)];
                a.z-=10;if(a.z<1000) a.z+=4000;
            } else a.z+=4000;
        }
    } else if(scene_==Scene::Base) {canvas_=background_;sprite(3,pose_,154,y_);}
    colors();
}
void Presentation::step(unsigned ticks,bool retrace,bool compose,bool escape) {
    if(scene_==Scene::Done) return;
    ticks_=(ticks_+ticks)&65535;
    auto next=[&](){enter(static_cast<Scene>(unsigned(scene_)+1));};
    if(cut_) {cut_=false;ticks_=0;return;}
    if(movie(scene_)) {
        if(waiting_) {
            if(escape_ && escape) {next();return;}
            if(ticks_<=stage(scene_).interval) return;
            ticks_=0;waiting_=false;
            if(++frame_==stage(scene_).frames) {escape_=false;next();return;}
        }
        if(retrace) {draw();waiting_=true;ticks_=0;} return;
    }
    if(scene_==Scene::Base) {
        if(fade_) {
            if(retrace) {colors(brightness_);if(brightness_==0) next();else brightness_-=2;}return;
        }
        if(!waiting_) {draw();waiting_=true;ticks_=0;return;}
        if(ticks_<=10) return;
        ticks_=0;constexpr int deltas[]{-1,-3,-2,-1,0,0,-2,-2,-1,-1,-1,-1};
        y_+=deltas[delta_++%12];pose_+=direction_;if(pose_==7) direction_=-1;if(pose_==1) direction_=1;
        if(y_<=100) {cues_.push_back(-1);fade_=true;brightness_=100;} else {draw();}return;
    }
    if(!compose) return;
    draw();
    if(scene_==Scene::Hangar) {
        if(ticks_>26) {ticks_=0;++hangarStep_;if(++c_==4) c_=2;if(hangarStep_==36) next();}
    } else {
        tick_=false;if(ticks_>20) {if(scene_==Scene::Cruise) total_=(total_+ticks_)&65535;ticks_=0;tick_=true;}
        if(textDone_ && ((scene_==Scene::Startup && script_==1) || (scene_==Scene::Cruise && total_>4800))) next();
    }
}
void Presentation::advance(double seconds,bool escape) {
    pit_+=std::max(0.0,seconds)*(1193182.0/7457.0);unsigned ticks=unsigned(pit_);pit_-=ticks;
    retrace_+=seconds*70;cpu_+=seconds*70;
    bool retrace=retrace_>=1,compose=cpu_>=1;
    if(retrace) retrace_=0;if(compose) cpu_=0;
    step(ticks,retrace,compose,escape);
}
}
