#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include "extremis/cinematics/original_ending.h"
#include "app/session/victory_session.h"
#include "extremis/resources/audio_data.h"
#ifdef EXTREMIS_ENDING_PREVIEW
#include "extremis/cinematics/original_ending_player.h"
#include <SDL3/SDL.h>
#endif
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <utility>

using namespace original_ending;
namespace {
void check(bool ok,const char* text) { if(!ok) throw std::runtime_error(text); }
Resources fixture() {
    Resources r;const unsigned counts[]{16,1,12,1,52,12};
    for(unsigned i=0;i<6;++i) {
        auto& a=r.visuals[i];a.palette.fill(0xffffffff);a.palette[0]=255;
        for(unsigned f=0;f<counts[i];++f) a.frames.emplace_back(64000,std::uint8_t(1+i*20+f));
    }
    for(unsigned bank=0;bank<2;++bank)
        for(unsigned i=0;i<(bank?13u:7u);++i) {
            spr_format::Image s{bank?230:17,bank?49:23,0,0,{}};
            s.indices.assign(s.width*s.height,100+i);s.indices[0]=0;
            r.sprites[bank].push_back(std::move(s));
        }
    return r;
}
void reach(Presentation& p,Scene s,Phase phase) {
    for(unsigned i=0;i<20000 && (p.scene()!=s || p.phase()!=phase);++i) p.step(1,true);
    check(p.scene()==s && p.phase()==phase,"Expected ending scene/phase unreachable");
}
void firstWait(Presentation& p) {
    for(unsigned i=0;i<26;++i) p.step(0,true);
    check(p.scene()==Scene::Fseq1 && p.phase()==Phase::Frame,"Gold transition length");
    p.step(0,true);
    for(unsigned i=0;i<26;++i) p.step(0,true);
    check(p.phase()==Phase::Wait && p.frame()==0,"First reveal length");
}
void fullSequence(Resources r) {
    VictorySession victory; unsigned plays=0, menuEntries=0;
    const auto play=[&]() {
    ++plays;
    Presentation p(std::move(r));
    std::vector<Scene> scenes{p.scene()};
    std::array<std::vector<std::pair<unsigned,unsigned>>,4> movies;
    std::vector<unsigned> descent,poses;
    std::vector<std::tuple<unsigned,Sound,unsigned>> cues;
    unsigned holds=0,fades=0;
    for(unsigned i=0;i<30000 && p.scene()!=Scene::Done;++i) {
        const auto s=p.scene();
        int movie=s==Scene::Fseq1?0:s==Scene::Fseq3?1:s==Scene::Fseq5?2:s==Scene::Fseq6?3:-1;
        if(movie>=0 && (p.phase()==Phase::Wait || p.phase()==Phase::Reveal)) {
            const auto f=std::make_pair(p.pass(),p.frame());auto& list=movies[movie];
            if(list.empty() || list.back()!=f) list.push_back(f);
        }
        if((s==Scene::Fseq2 || s==Scene::Fseq4Poses) && p.phase()==Phase::Wait) {
            auto& list=s==Scene::Fseq2?descent:poses;
            if(list.empty() || list.back()!=p.pose()) list.push_back(p.pose());
        }
        if(s==Scene::Hold) ++holds;
        if(s==Scene::Fade) ++fades;
        p.step(1,true);
        for(const auto& c:p.takeCues()) cues.emplace_back(c.voice,c.sound,c.volume);
        if(p.scene()!=scenes.back()) scenes.push_back(p.scene());
    }
    check(scenes==std::vector<Scene>{Scene::Gold,Scene::Fseq1,Scene::Fseq2,Scene::Fseq3,Scene::Fseq4Move,
        Scene::Fseq4Poses,Scene::Fseq5,Scene::Fseq6,Scene::Hold,Scene::Fade,Scene::Done},"Ending scene order");
    check(movies[0].size()==64 && movies[1].size()==12 && movies[2].size()==52 && movies[3].size()==12,"Missing movie frames/repeats");
    for(unsigned i=0;i<64;++i) check(movies[0][i]==std::make_pair(1+i/16,i%16),"FSEQ1 must play four complete passes");
    for(unsigned m=1;m<4;++m) for(unsigned f=0;f<movies[m].size();++f) check(movies[m][f].second==f,"Movie frame order");
    check(descent.size()==48 && descent.front()==0 && descent.back()==47,"FSEQ2 draw count");
    check(poses.size()==13 && poses.front()==0 && poses.back()==12,"FSEQ4 pose count");
    check(holds==350 && fades==101,"Final hold/fade must count retraces, not PIT ticks");
    const std::vector<std::tuple<unsigned,Sound,unsigned>> expected{
        {0,Sound::Monster,64},{0,Sound::None,0},{0,Sound::Wind,48},
        {0,Sound::None,0},{0,Sound::Wind,32},{0,Sound::None,0},{0,Sound::Labo,48},
        {0,Sound::None,0},{0,Sound::Barge,48},{0,Sound::None,0},{0,Sound::Radar,48},
        {1,Sound::Radar,48},{1,Sound::None,0},{0,Sound::None,0}};
    check(cues==expected,"Ending sound ownership, restart, volume or stop ordering");
    check(std::all_of(p.pixels().begin(),p.pixels().end(),[](auto c){return c==255;}),"Ending must finish black");
    check(std::all_of(p.indices().begin(),p.indices().end(),[](auto c){return c==0;}),"Ending must clear canvas");
    p.advance(100,true);check(p.takeCues().empty() && p.scene()==Scene::Done,"Completion must be idempotent");
    check(menuEntries==0,"Menu appeared before FSEQ completed");
    return StartupCreditsResult::Continue;
    };
    const auto enter=[&]() { ++menuEntries; };
    victory.update(false,play,enter);check(plays==0,"Premature FSEQ");
    victory.update(true,play,enter);
    victory.update(true,play,enter);
    check(plays==1 && menuEntries==1,"Full FSEQ must return to menu exactly once");
    for(auto outcome:{StartupCreditsResult::Quit,StartupCreditsResult::Error}) {
        victory.reset();
        check(victory.update(true,[&]() { return outcome; },enter)==outcome && menuEntries==1,
              "Interrupted FSEQ must propagate its outcome without opening the menu");
    }
}
void timing() {
    Presentation p(fixture());firstWait(p);
    p.step(10,false);check(p.frame()==0,"Movie wait is strict >10");
    p.step(1,false);check(p.phase()==Phase::Frame,"Elapsed wait must still await retrace");
    p.step(0,true);check(p.frame()==1 && p.phase()==Phase::Wait,"Movie frame retrace");
    p.step(1000,true);check(p.frame()==2,"Late host must not skip frames");
    p.step(0,true);check(p.frame()==2,"Wait must reset excess ticks");
    for(unsigned i=0;i<13;++i) p.step(11,true);
    check(p.frame()==15 && p.pass()==1,"Last movie frame missing");
    p.step(10,true);check(p.frame()==15,"Final frame needs full wait");
    p.step(1,true);check(p.frame()==0 && p.pass()==2 && p.phase()==Phase::Wait,"Repeat must use direct palette, no second reveal");
    reach(p,Scene::Fseq3,Phase::Cut);p.step(0,true);
    p.step(13,true);check(p.frame()==0,"FSEQ3 strict >13");p.step(1,true);check(p.frame()==1,"FSEQ3 threshold");
    reach(p,Scene::Fseq5,Phase::Cut);p.step(0,true);
    p.step(12,true);check(p.frame()==0,"FSEQ5 strict >12");p.step(1,true);check(p.frame()==1,"FSEQ5 threshold");
    reach(p,Scene::Fseq6,Phase::Cut);p.step(0,true);
    check(p.phase()==Phase::Reveal && p.pixels()[100]==255,"FIN must reveal from black");
    for(unsigned i=0;i<26;++i) p.step(0,true);
    p.step(13,true);check(p.frame()==0,"FSEQ6 strict >13");p.step(1,true);check(p.frame()==1,"FSEQ6 threshold");
    reach(p,Scene::Hold,Phase::Hold);
    p.step(30000,false);check(p.scene()==Scene::Hold,"PIT time must not consume final retraces");
    for(unsigned i=0;i<349;++i) p.step(0,true);
    check(p.scene()==Scene::Hold,"350th retrace required");p.step(0,true);check(p.scene()==Scene::Fade,"Hold did not finish");
    p.step(0,true);check(p.pixels()[100]==0xffffffff,"Fade must begin at 100");
    p.step(0,true);check(p.pixels()[100]==0xfbfbfbff,"Fade must decrement by one DAC brightness percent");
    Presentation reveal(fixture());
    for(unsigned i=0;i<27;++i) reveal.step(0,true);
    for(unsigned i=0;i<26;++i) reveal.step(2,true);
    reveal.step(0,true);check(reveal.frame()==1,"Reveal must share first-frame PIT interval");
}
void graphics() {
    EntryImage rgba; rgba.rgba.assign(64000,0x408020ff);
    EntryImage indexed; indexed.palette.fill(0x408020ff);
    Presentation a(fixture(),rgba), b(fixture(),indexed);
    for(unsigned i=0;i<26;++i) {
        check(a.pixels()==b.pixels(),"RGBA frozen frame tint differs from indexed tint");
        a.step(0,true); b.step(0,true);
    }
    EntryImage entry;entry.indices.assign(64000,9);entry.palette.fill(0xffffffff);
    Presentation p(fixture(),entry);
    check(p.indices()[0]==9 && p.pixels()[0]==0xffffffff,"Entry must preserve supplied image/palette");
    p.step(0,true);check(p.pixels()[0]==0xffffffff,"Gold starts at amount zero");
    p.step(0,true);check(p.pixels()[0]==0xfbf7f3ff,"Golden blend must truncate source and target independently");
    for(unsigned i=2;i<26;++i) p.step(0,true);
    check(p.pixels()[0]==0xffc200ff,"Gold endpoint DAC (63,48,0)");
    reach(p,Scene::Fseq2,Phase::Cut);check(p.pixels()[0]==255,"Scene boundary must black palette");p.step(0,false);
    for(unsigned i=0;i<48;++i) {
        const unsigned q=i%12,color=100+(q<=6?q:12-q);const int y=102+i;
        check(p.indices()[y*320+154]==21 && p.indices()[y*320+155]==color,"FSEQ2 position/ping-pong/transparency");
        if(i) check(p.indices()[(y-1)*320+155]==21,"FSEQ2 background must be restored");
        p.step(10,false);check(p.scene()==Scene::Fseq2 && p.pose()==i,"FSEQ2 strict >10");p.step(1,false);
    }
    check(p.scene()==Scene::Fseq3,"FSEQ2 must not draw y=150");
    reach(p,Scene::Fseq4Move,Phase::Cut);p.step(0,false);p.step(160,true);
    check(p.indices()[24*320+274]==61 && p.indices()[24*320+275]==100 && p.indices()[24*320+319]==100,"Clipped FSEQ4 entrance");
    p.step(0,true);check(p.indices()[24*320+177]==100 && p.indices()[24*320+175]==61,"Signed translation at t=160");
    p.step(160,true);check(p.scene()==Scene::Fseq4Poses,"FSEQ4 movement endpoint");
    const int xy[][2]{{77,24},{78,24},{78,24},{79,23},{81,23},{82,23},{85,23},
        {87,22},{90,22},{84,22},{77,21},{69,21},{62,21}};
    for(unsigned i=0;i<13;++i) {
        const auto pos=xy[i][1]*320+xy[i][0];
        check(p.indices()[pos]==61 && p.indices()[pos+1]==100+i,"FSEQ4 original position/sprite table");
        p.step(31,false);check(p.pose()==i,"FSEQ4 pose must wait >=32");p.step(1,false);
    }
    check(p.scene()==Scene::Fseq5,"FSEQ4 final pose must also wait");
}
void escapeAndClock() {
    Presentation a(fixture()),b(fixture());
    for(unsigned i=0;i<10000 && a.scene()!=Scene::Done;++i) {
        a.advance(1.0/70,true);b.advance(1.0/70,false);
        check(a.scene()==b.scene() && a.phase()==b.phase() && a.frame()==b.frame(),"Escape altered sequence");
    }
    check(a.scene()==Scene::Done,"Host-clock playback failed to complete");
    Presentation p(fixture());p.advance(-1,true);p.advance(std::numeric_limits<double>::quiet_NaN());
    check(p.scene()==Scene::Gold && p.amount()==0,"Invalid clock advanced presentation");
}
void malformed() {
    auto reject=[](Resources r) {try {Presentation p(std::move(r));} catch(const std::runtime_error&) {return;}throw std::runtime_error("Malformed resource accepted");};
    auto r=fixture();r.visuals[0].frames.pop_back();reject(std::move(r));
    r=fixture();r.visuals[5].frames[2].resize(1);reject(std::move(r));
    r=fixture();r.sprites[0].pop_back();reject(std::move(r));
    r=fixture();r.sprites[1][12].indices.clear();reject(std::move(r));
    r=fixture();r.sprites[1][0].anchorX=1;reject(std::move(r));
    try {EntryImage e;e.indices.clear();Presentation p(fixture(),e);} catch(const std::runtime_error&) {return;}
    throw std::runtime_error("Invalid entry image accepted");
}
#ifdef EXTREMIS_ENDING_PREVIEW
int play(const char* directory) {
    if(!SDL_Init(SDL_INIT_VIDEO)) throw std::runtime_error(SDL_GetError());
    SDL_Window* window=nullptr;SDL_Renderer* renderer=nullptr;
    if(!SDL_CreateWindowAndRenderer("Extremis RE — FSEQ Debug",960,600,SDL_WINDOW_RESIZABLE,&window,&renderer)) {
        const std::string error=SDL_GetError();SDL_Quit();throw std::runtime_error(error);
    }
    const auto result=showOriginalEnding(window,renderer,directory);
    SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();
    return result==StartupCreditsResult::Error?1:0;
}
#endif
}
int main(int argc,char** argv) {
    try {
#ifdef EXTREMIS_ENDING_PREVIEW
        if(argc==3 && std::string(argv[1])=="--play") return play(argv[2]);
#endif
        if(argc>2 || (argc==2 && std::string(argv[1]).rfind("--",0)==0)) throw std::runtime_error("Expected optional original DATA directory");
        fullSequence(fixture());timing();graphics();escapeAndClock();malformed();
        if(argc==2) {
            fullSequence(loadResources(argv[1]));
            for(unsigned i=1;i<unsigned(Sound::Count);++i) {
                const auto s=audio::loadSample(std::filesystem::path(argv[1])/soundFile(Sound(i)));
                check(s.rate==5000 && !s.pcm.empty(),"Invalid original ending sound");
                check(bool(s.loopFlag)==(i<=2),"Original ending sample loop mismatch");
            }
        }
        std::cout<<"FSEQ scenes/repeats/palette/motion/timing/audio/Escape/cleanup tests passed"<<(argc==2?" with original resources":"")<<'\n';
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
