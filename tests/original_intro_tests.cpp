#include "extremis/cinematics/original_intro.h"
#include "extremis/ui/localization.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace original_intro;
namespace {
constexpr double tick=7457.0/1193182.0;
template<class E> unsigned ix(E e) {return static_cast<unsigned>(e);}
void check(bool value,const char* message) {if(!value) throw std::runtime_error(message);}
Resources fixture() {
    Resources r;
    for(auto& v:r.visuals) {v.palette.fill(0x8090a0ff);v.palette[0]=255;v.frames.assign(1,std::vector<std::uint8_t>(64000,1));}
    for(auto pair:{std::pair<Visual,unsigned>{Visual::Ship,16},{Visual::Radar,36},{Visual::Labo,61},{Visual::Message,21},{Visual::Alarm,12}})
        r.visuals[ix(pair.first)].frames.assign(pair.second,std::vector<std::uint8_t>(64000,1));
    constexpr unsigned counts[]{37,4,7,7,2,1,9,5,76};
    for(unsigned i=0;i<r.sprites.size();++i)
        for(unsigned j=0;j<counts[i];++j) r.sprites[i].push_back({1,1,0,0,{std::uint8_t(j+2)}});
    r.text={std::string(350,'A'),std::string(316,'B')};
    return r;
}
void reach(Presentation& p,Scene scene) {
    for(unsigned i=0;p.scene()!=scene && i<20000;++i) p.advance(1.0/70);
    check(p.scene()==scene,"Scene unreachable");
}
void sequence(Resources r) {
    Presentation p(std::move(r));
    std::vector<Scene> scenes{p.scene()};
    unsigned cabinStarts=0,radarStarts=0,radarPasses=0,touchStarts=0,explosions=0,messageTail=0;
    unsigned panelRecords=0,transferRecords=0,lastRecord=999;
    for(unsigned i=0;i<20000 && p.scene()!=Scene::AudioWait;++i) {
        for(const auto& c:p.takeCues()) {
            check(c.voice<2 && c.volume<=64,"Invalid voice cue");
            if(c.sound==Sound::Cabin) {++cabinStarts;check(c.voice==0 && c.volume==48,"Cabin cue");}
            if(c.sound==Sound::Radar) {++radarStarts;check(c.voice==1 && c.volume==48,"Radar cue");}
            if(c.sound==Sound::Touch) ++touchStarts;
            if(c.sound==Sound::Explosion) {++explosions;check(c.voice==0 && c.volume==64,"Explosion cue");}
        }
        if(p.scene()==Scene::Radar) radarPasses=std::max(radarPasses,p.moviePass());
        if(p.scene()==Scene::Message && p.frame()==21) messageTail=std::max(messageTail,p.textPosition());
        if(p.scene()==Scene::Panel || p.scene()==Scene::Transfer) {
            const unsigned key=ix(p.scene())*100+p.record();
            if(key!=lastRecord) {lastRecord=key;if(p.scene()==Scene::Panel) ++panelRecords;else ++transferRecords;}
        }
        const auto before=p.scene();p.advance(1.0/70);
        if(p.scene()!=before) scenes.push_back(p.scene());
    }
    check(p.scene()==Scene::AudioWait,"Full intro did not reach audio wait");
    check(scenes.size()==14,"Missing or extra intro stage");
    for(unsigned i=0;i<scenes.size();++i) check(ix(scenes[i])==i,"Original scene order changed");
    check(radarStarts==5 && radarPasses==5,"PSEQ3 must repeat to text completion (five resource-backed passes)");
    check(cabinStarts==2,"CABINE must start in cockpit and once on radar entry");
    check(touchStarts==6 && explosions==1,"Scene sound requests/restarts changed");
    check(messageTail==316,"PSEQ6 text continuation incomplete");
    check(panelRecords==13 && transferRecords==9,"Motion records missing");
    check(std::all_of(p.pixels().begin(),p.pixels().end(),[](auto c){return c==255;}),"Final fade not black");
    p.advance(10,true,true);check(p.scene()==Scene::AudioWait,"Final voice wait must ignore Escape");
    p.advance(0,false,false);check(p.scene()==Scene::Done,"Finished voice must release intro");
    const auto stop=p.takeCues();
    check(stop.size()==2 && stop[0].voice==1 && stop[1].voice==0 &&
          stop[0].sound==Sound::None && stop[1].sound==Sound::None,"Final voice cleanup");
}
void textAndTiming() {
    auto r=fixture();r.text[0]="A\r\nB C"; // even, byte-based callback; CR/LF count as bytes
    Presentation p(r);reach(p,Scene::Radar);
    check(p.textPosition()==2 && p.indices()[10*320+10]==1,"Initial callback draws empty prefix then increments");
    p.advance(8*tick);check(p.frame()==0 && p.textPosition()==2,"ANM threshold must be strictly >8 ticks");
    p.advance(tick); // retrace may still be pending
    p.advance(1.0/70);
    check(p.frame()==1 && p.textPosition()==4,"Two bytes per ANM callback");
    check(p.indices()[10*320+10]==12,"Font mapping/width (A is DOS sprite 11)");
    p.advance(.1);
    check(p.frame()==2 && p.textPosition()==6 && p.indices()[21*320+10]==13,"CR/LF handling or text position");
    for(unsigned i=0;i<100 && p.scene()==Scene::Radar;++i) p.advance(.1);
    check(p.scene()==Scene::Labo && p.moviePass()==1,"Text completion must finish whole PSEQ3 movie, not restart blindly");
    Presentation slow(fixture());reach(slow,Scene::Radar);
    slow.advance(1);check(slow.frame()==1,"Long host frame must not catch up/skips ANM frames");
    const auto frame=slow.frame();slow.advance(0);slow.advance(-1);slow.advance(std::numeric_limits<double>::quiet_NaN());
    slow.advance(std::numeric_limits<double>::infinity());check(slow.frame()==frame,"Invalid elapsed time advances intro");
}
void escapeBoundaries() {
    Presentation skipped(fixture(),true);
    check(skipped.scene()==Scene::Done,"Entry Escape must skip PSEQ before scene start");
    const auto skippedCues=skipped.takeCues();
    check(std::none_of(skippedCues.begin(),skippedCues.end(),[](const Cue& c){return c.sound==Sound::Barge;}),"Entry Escape must not start BARGE2");
    Presentation p(fixture());p.advance(0,true);check(p.scene()==Scene::Ship,"Fade-in must not poll Escape");
    for(unsigned i=0;i<30 && p.scene()!=Scene::Done;++i) p.advance(1.0/70);
    check(p.scene()==Scene::Done,"Queued Escape lost during reveal");
    for(auto scene:{Scene::Cockpit,Scene::Radar,Scene::Labo,Scene::CockpitAlarm,Scene::Stars,Scene::Alarm,Scene::Depart,Scene::Explosion}) {
        Presentation q(fixture());reach(q,scene);q.advance(0,true);check(q.scene()==Scene::Done,"Scene Escape boundary");
    }
    for(auto scene:{Scene::Panel,Scene::Transfer}) {
        Presentation q(fixture());reach(q,scene);q.takeCues();
        q.advance(0,true);check(q.scene()==scene,"Escape interrupted a motion record");
        q.advance((scene==Scene::Panel?39.0:19.0)*tick);check(q.scene()==scene,"Motion boundary early");
        q.advance(tick);check(q.scene()==Scene::Done,"Escape not consumed at record boundary");
    }
    Presentation q(fixture());reach(q,Scene::Message);
    while(q.frame()<21) q.advance(.1);
    q.advance(0,true);check(q.scene()==Scene::Message,"Escape interrupted PSEQ6 busy wait");
    q.advance(14*tick);check(q.scene()==Scene::Message,"PSEQ6 >=15 wait too short");
    q.advance(tick);check(q.scene()==Scene::Done,"PSEQ6 wait Escape lost");
    Presentation f(fixture());reach(f,Scene::Fade);f.advance(0,true,true);
    check(f.scene()==Scene::Fade,"Final fade polls Escape");
}
void graphics() {
    auto r=fixture();
    r.visuals[ix(Visual::Cockpit)].frames[0].assign(64000,1);
    r.visuals[ix(Visual::CockpitFlash)].frames[0].assign(64000,100);
    // Transparent zero leaves the restored background, anchorless helper ignores anchor.
    r.sprites[ix(Bank::Cockpit)][16]={2,1,1,0,{0,99}};
    Presentation p(r);reach(p,Scene::Cockpit);
    check(p.indices()[70*320+129]==1 && p.indices()[70*320+130]==99,"Cockpit transparent/anchorless blit");
    reach(p,Scene::CockpitAlarm);p.advance(20.1*tick);
    check(p.indices()[0]==100,"Cockpit flashing VGA selection");
    r=fixture();r.sprites[ix(Bank::Hand)][0]={320,200,0,0,std::vector<std::uint8_t>(64000,0)};
    r.sprites[ix(Bank::Hand)][0].indices[0]=200;
    Presentation edge(r);reach(edge,Scene::Panel);
    check(edge.indices()[112*320+297]==200 && edge.indices().size()==64000,"Offscreen sprite clipping");
    for(unsigned i=0;i<300 && edge.scene()==Scene::Panel;++i) edge.advance(.02);
    check(edge.scene()==Scene::Depart,"Offscreen motion failed");
    // Scaling from 82x103 to smaller sizes must clip/source-bound correctly.
    r.sprites[ix(Bank::Depart)][0]={82,103,0,0,std::vector<std::uint8_t>(82*103,77)};
    Presentation scale(r);reach(scale,Scene::Depart);
    check(scale.indices()[30*320+72]==77 && scale.indices()[132*320+153]==77 && scale.indices()[133*320+154]==1,"Initial scaled bounds");
    scale.advance(160.1*tick);
    check(scale.indices()[47*320+101]==77 && scale.indices()[117*320+101]==1,"Scaled interpolation bounds");
    reach(scale,Scene::Transfer);
    const auto background=scale.indices();
    scale.advance(.05);check(scale.indices()==background,"PSEQ12 must not invent a sprite from the mismatched buffer");
    reach(scale,Scene::Explosion);
    // Keep first VGA palette even when selecting the second VGA pixels.
    r.visuals[ix(Visual::ExplosionFlash)].frames[0].assign(64000,9);
    r.visuals[ix(Visual::Explosion)].palette[9]=0xff0000ff;
    r.visuals[ix(Visual::ExplosionFlash)].palette[9]=0x00ff00ff;
    Presentation flash(r);reach(flash,Scene::Explosion);flash.takeCues();
    flash.advance(330.1*tick);check(flash.indices()[0]==9 && flash.pixels()[0]==0xff0000ff,"Explosion flash palette changed");
    auto cues=flash.takeCues();check(cues.size()==2 && cues[0].sound==Sound::Explosion && cues[1].sound==Sound::None,"Explosion threshold cue");
    flash.advance(1.0/70);check(flash.takeCues().empty(),"Explosion replayed every frame");
}
void invalidResources() {
    for(unsigned kind=0;kind<8;++kind) {
        auto r=fixture();
        switch(kind) {
        case 0:r.visuals[0].frames.clear();break;
        case 1:r.visuals[0].frames[0].pop_back();break;
        case 2:r.sprites[0].pop_back();break;
        case 3:r.sprites[0][0].width=-1;break;
        case 4:r.sprites[0][0].indices.clear();break;
        case 5:r.sprites[0][0].anchorX=std::numeric_limits<int>::max();break;
        case 6:r.text[0].push_back('x');break;
        case 7:r.text[0][0]=0;break;
        }
        bool rejected=false;try {Presentation p(std::move(r));} catch(const std::exception&) {rejected=true;}
        check(rejected,"Invalid resource accepted");
    }
}
void localizedRadar() {
    localization::Language language=localization::Language::English;
    localization::bind(&language);
    Presentation english(fixture());reach(english,Scene::Radar);
    english.advance(9*tick);
    const auto source=english.indices();
    const auto position=english.textPosition(),frame=english.frame();
    language=localization::Language::Spanish;
    Presentation spanish(fixture());reach(spanish,Scene::Radar);
    spanish.advance(9*tick);
    check(spanish.frame()==frame && spanish.textPosition()==position,"Intro localization changed source clock");
    check(spanish.indices()!=source,"Spanish radar text was not rendered");
    localization::bind(nullptr);
}
}
int main(int argc,char** argv) {
    try {
        sequence(fixture());textAndTiming();escapeBoundaries();graphics();invalidResources();localizedRadar();
        if(argc==2) sequence(loadResources(argv[1]));
        std::cout<<"Original intro sequence/text/input/graphics tests passed"<<(argc==2?" (including local original resources)":"")<<"\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
