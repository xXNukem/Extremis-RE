#include "extremis/cinematics/original_presentation.h"
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>

using namespace original_presentation;
namespace {
constexpr double tick=7457.0/1193182.0,pulse=1.0/70;
void check(bool ok,const char* message) {if(!ok) throw std::runtime_error(message);}
Resources fixture() {
    Resources r;
    r.conceptPalette.fill(0xffffffff);r.conceptPalette[0]=255;
    r.conceptSprite={133,82,3,4,std::vector<std::uint8_t>(133*82,2)};
    r.conceptSprite.indices[0]=0;
    const auto make=[&](unsigned count,unsigned color) {
        anm_format::Animation a;a.palette=r.conceptPalette;
        for(unsigned i=0;i<count;++i) a.frames.emplace_back(64000,std::uint8_t(color+i));
        return a;
    };
    r.concept=make(24,10);r.title=make(18,40);
    for(unsigned i=0;i<5;++i) r.cards[i]=make(1,70+i);
    return r;
}
void reach(Presentation& p,Screen screen,Phase phase) {
    for(unsigned i=0;i<10000 && (p.screen()!=screen || p.phase()!=phase);++i) p.advance(pulse);
    check(p.screen()==screen && p.phase()==phase,"Expected screen/phase unreachable");
}
void fullSequence(Resources r) {
    Presentation p(std::move(r));
    std::vector<Screen> screens{p.screen()};
    std::vector<std::pair<unsigned,unsigned>> concept,title;
    unsigned holds=0,holdPulses=0,cardHolds=0;
    auto previousScreen=p.screen();auto previousPhase=p.phase();
    for(unsigned i=0;i<20000 && p.screen()!=Screen::Done;++i) {
        if(p.screen()==Screen::LogoMovie || p.screen()==Screen::Title) {
            auto& frames=p.screen()==Screen::Title?title:concept;
            const auto current=std::make_pair(p.pass(),p.frame());
            if(frames.empty() || frames.back()!=current) frames.push_back(current);
        }
        if(p.screen()==Screen::LogoHold) ++holdPulses;
        previousScreen=p.screen();previousPhase=p.phase();p.advance(pulse);
        if(previousScreen==Screen::LogoHold && p.screen()!=previousScreen) {
            check(holdPulses==70,"Logo must hold for 70 retraces after each playback");
            ++holds;holdPulses=0;
        }
        if(previousPhase==Phase::Hold && previousScreen!=Screen::LogoHold && p.phase()==Phase::FadeOut)
            ++cardHolds;
        if(p.screen()!=screens.back()) screens.push_back(p.screen());
    }
    const std::vector<Screen> expected{Screen::LogoIn,Screen::LogoMovie,Screen::LogoHold,
        Screen::LogoMovie,Screen::LogoHold,Screen::LogoOut,Screen::Presente,Screen::Title,
        Screen::Who1,Screen::Who2,Screen::Who3,Screen::Who4,Screen::Done};
    check(screens==expected,"Original startup order/repetition changed");
    check(concept.size()==48 && title.size()==36 && holds==2 && cardHolds==5,"Missing frames, passes or cards");
    for(unsigned i=0;i<48;++i) check(concept[i]==std::make_pair(1+i/24,i%24),"CONCEPT frame/pass order");
    for(unsigned i=0;i<36;++i) check(title[i]==std::make_pair(1+i/18,i%18),"TITRE frame/pass order");
    check(std::all_of(p.pixels().begin(),p.pixels().end(),[](auto c){return c==255;}),"Handoff must be black");
}
void timingAndGraphics() {
    Presentation p(fixture());
    check(p.brightness()==0 && p.pixels()[100]==255,"Logo entrance must start black");
    p.advance(80*tick+1e-9);
    check(p.screen()==Screen::LogoIn && p.brightness()==50,"Logo uses PIT-scaled brightness");
    check(p.indices()[199*320+319]==2 && p.pixels()[199*320+319]==0x7d7d7dff,"Clipped zoom and DAC integer brightness");
    reach(p,Screen::LogoHold,Phase::Hold);
    check(p.indices()[50*320+86]==0 && p.indices()[50*320+87]==2 && p.indices()[49*320+87]==0,
          "Logo hold must use sprite anchors and zero transparency");
    reach(p,Screen::LogoOut,Phase::Scale);
    check(p.brightness()==100,"Reverse logo begins fully lit");
    p.advance(80*tick+1e-9);check(p.brightness()==50,"Reverse logo scale/fade");
    reach(p,Screen::Presente,Phase::FadeIn);
    check(p.indices()[100]==70 && p.brightness()==0,"PRESENTE frame/fade entry");
    for(unsigned b=4;b<=100;b+=4) {p.advance(pulse);check(p.brightness()==b,"Card reveal step must be four");}
    p.advance(pulse);check(p.phase()==Phase::Hold,"Reveal must complete before hold");
    p.advance(800*tick);check(p.phase()==Phase::Hold,"Card timeout must be strictly >800");
    p.advance(tick+1e-9);check(p.phase()==Phase::FadeOut,"Card timeout did not start fade");
    for(unsigned b=96;;b-=4) {p.advance(pulse);check(p.brightness()==b,"Card fade-out step must be four");if(b==0)break;}
    p.advance(pulse);check(p.screen()==Screen::Title && p.brightness()==0,"Title must start from black");
    for(unsigned b=4;b<=100;b+=4) {p.advance(pulse);check(p.frame()==0 && p.brightness()==b,"Title first frame reveal");}
    reach(p,Screen::Title,Phase::Movie);
    p.advance(pulse);check(p.frame()==1,"Title reveal shares first frame wait counter");
    for(unsigned i=0;i<1000 && p.pass()==1;++i) p.advance(pulse);
    check(p.screen()==Screen::Title && p.pass()==2 && p.phase()==Phase::FadeIn && p.brightness()==0,
          "Second title play must also fade in");
    Presentation slow(fixture());reach(slow,Screen::LogoMovie,Phase::Movie);
    slow.advance(1);check(slow.frame()==1,"Late host frame must not skip movie frames");
    slow.advance(-1);slow.advance(std::numeric_limits<double>::quiet_NaN());
    check(slow.frame()==1,"Invalid elapsed time advanced movie");
}
void keyboard() {
    Presentation p(fixture());p.advance(0,true); // Includes Escape: same any-key condition.
    check(p.screen()==Screen::LogoIn,"Key must not skip logo zoom");
    reach(p,Screen::Presente,Phase::Hold);p.advance(0);
    check(p.phase()==Phase::FadeOut,"Key from uninterruptible logo must remain pending for card");
    reach(p,Screen::Title,Phase::FadeIn);p.advance(0,true);
    check(p.phase()==Phase::FadeIn && p.pass()==1,"Key must not interrupt title reveal");
    reach(p,Screen::Who1,Phase::Hold);p.advance(0);
    check(p.phase()==Phase::FadeOut,"Title key must advance first credit after both plays");
    reach(p,Screen::Who2,Phase::Hold);p.advance(0);
    check(p.phase()==Phase::Hold,"Consumed key must not advance a second card");
    p.advance(0,true);check(p.screen()==Screen::Who2 && p.phase()==Phase::FadeOut,"Key must fade current card, not skip block");
    p.advance(0,true); // Another key during uninterruptible fade belongs to next hold.
    reach(p,Screen::Who3,Phase::Hold);p.advance(0);
    check(p.phase()==Phase::FadeOut,"Fade key must remain pending");
    reach(p,Screen::Who4,Phase::FadeIn);p.advance(0,true);
    check(p.phase()==Phase::FadeIn,"Key must not skip last credit reveal");
    reach(p,Screen::Who4,Phase::Hold);p.advance(0);
    check(p.phase()==Phase::FadeOut,"Last credit key must initiate fade");
    reach(p,Screen::Done,Phase::FadeOut);
}
void malformed() {
    auto r=fixture();r.concept.frames.pop_back();
    try {Presentation p(std::move(r));throw std::logic_error("Accepted missing CONCEPT frame");}
    catch(const std::runtime_error&) {}
    r=fixture();r.cards[2].frames[0].resize(12);
    try {Presentation p(std::move(r));throw std::logic_error("Accepted incomplete credit canvas");}
    catch(const std::runtime_error&) {}
}
}
int main(int argc,char** argv) {
    try {
        fullSequence(fixture());timingAndGraphics();keyboard();malformed();
        if(argc>1) fullSequence(loadResources(argv[1]));
        std::cout<<"Original startup order/frames/fades/timing/keyboard tests passed"<<(argc>1?" with original resources":"")<<"\n";
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
