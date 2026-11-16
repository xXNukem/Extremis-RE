#include "level22_communication.h"
#include "door_control.h"
#include "item_session.h"
#include "survival_session.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace communication;
namespace {
constexpr double pulse=1.0/70,tick=7457.0/1193182;
void check(bool ok,const char* message) {if(!ok) throw std::runtime_error(message);}
Resources fixture() {
    Resources r;
    for(auto* a:{&r.prompt,&r.first,&r.second}) {
        a->palette.fill(0x8090a0ff);a->palette[0]=255;a->frames.assign(1,std::vector<std::uint8_t>(64000,1));
    }
    r.font.assign(76,{3,10,0,0,std::vector<std::uint8_t>(30,2)});
    r.sprites.assign(78,{3,5,0,0,std::vector<std::uint8_t>(15,3)});
    for(unsigned i=69;i<77;++i) r.sprites[i]={48,64,0,0,std::vector<std::uint8_t>(48*64,std::uint8_t(i))};
    r.sprites[77]={31,11,0,0,std::vector<std::uint8_t>(31*11,5)};
    r.question={"Test?","Yes No!","No!"};
    for(unsigned i=0;i<7;++i) r.pages[i]=std::string(i+1,'A'+i)+"\rB";
    return r;
}
void reach(Presentation& p,Phase target,Input input={}) {
    for(unsigned i=0;i<20000 && p.phase()!=target && p.phase()!=Phase::Done;++i) p.advance(pulse,input);
    check(p.phase()==target,"Expected communication phase unreachable");
}
void full(Resources r) {
    Presentation p(std::move(r));reach(p,Phase::Choice);
    check(p.yes(),"YES must be default");p.advance(0,{0,true,false});
    reach(p,Phase::Typing);
    for(unsigned page=0;page<7;++page) {
        check(p.page()==page,"Page order changed");
        reach(p,Phase::Acknowledge);
        check(p.characters()>0,"Page text omitted");
        p.advance(30);check(p.phase()==Phase::Acknowledge,"Page timed out");
        p.advance(0,{0,true,false});
    }
    check(p.phase()==Phase::Closing,"Last page omitted closing animation");
    reach(p,Phase::Done);
    for(auto c:p.pixels()) check(c==255,"Final fade not black");
}
void choicesAndEscape() {
    Presentation no(fixture());reach(no,Phase::Choice);
    no.advance(0,{-1,false,true});check(!no.yes() && no.phase()==Phase::Choice,"Escape accepted at prompt");
    no.advance(0,{1,false,false});check(no.yes(),"Up did not select YES");
    no.advance(0,{-1,true,false});reach(no,Phase::Done);check(no.page()==0,"NO entered recording");
    Presentation first(fixture());reach(first,Phase::Choice);first.advance(0,{0,true,false});
    reach(first,Phase::Closing,{0,false,true});check(first.page()==0 && first.characters()==0,"First-page Escape lost");
    reach(first,Phase::Done);
    Presentation delayed(fixture());reach(delayed,Phase::Choice);delayed.advance(0,{0,true,false});reach(delayed,Phase::Typing);
    delayed.advance(0,{0,true,true});check(delayed.phase()==Phase::Typing && delayed.characters()==1,"Input skipped typing");
    reach(delayed,Phase::Acknowledge,{0,false,true});
    delayed.advance(1,{0,false,true});check(delayed.phase()==Phase::Acknowledge,"Escape skipped acknowledgement");
    delayed.advance(0,{0,true,true});check(delayed.phase()==Phase::Closing && delayed.page()==1,"Escape not checked at next page");
    reach(delayed,Phase::Done);
    Presentation released(fixture());reach(released,Phase::Choice);released.advance(0,{0,true,false});reach(released,Phase::Typing);
    released.advance(pulse,{0,false,true});reach(released,Phase::Acknowledge);
    released.advance(0,{0,true,false});check(released.phase()==Phase::Typing && released.page()==1,"Released Escape incorrectly latched");
}
void timingAndPixels() {
    Presentation p(fixture());check(p.brightness()==0,"Prompt must start black");
    p.advance(pulse);check(p.brightness()==2,"Fade step");reach(p,Phase::Question);
    p.advance(8*tick);check(p.characters()==1,"Question must wait >8 ticks");
    p.advance(tick+1e-9);check(p.characters()==2,"Question character timing");
    reach(p,Phase::Choice);p.advance(0,{0,true,false});reach(p,Phase::Opening);
    p.advance(20*tick);check(p.phase()==Phase::Opening,"Opening wait");
    reach(p,Phase::Typing);
    p.advance(4*tick);check(p.characters()==1,"Page must wait >4 ticks");
    p.advance(tick+1e-9);check(p.characters()==2,"Page character timing");
    check(p.pixels()[19*320+96]==0x8090a0ff && p.pixels()[19*320+100]==255,"Text position / cleared rectangle");
    auto pixels=p.pixels();auto chars=p.characters();
    p.advance(-1);p.advance(std::numeric_limits<double>::infinity());p.advance(0);
    check(p.characters()==chars && p.pixels()==pixels,"Invalid or paused time advanced presentation");
}
void triggerAndCampaign() {
    game_lab::Level level{};level.zoneIndex=21;level.connectionStates.resize(46);level.accessCode=123;
    for(unsigned word:{0u,0x100u,0x13cu,0x3du,0x180u,0x1ffu}) {
        level.connectionStates[44]=word;check(!trigger(level,16),"Premature trigger");
    }
    level.connectionStates[44]=0x13d;
    check(!trigger(level,44) && !trigger(level,15),"Wrong sector");
    level.zoneIndex=20;check(!trigger(level,16),"Wrong level");level.zoneIndex=21;
    level.connectionStates[44]=0;level.connectionStates[45]=0x13d;check(!trigger(level,16),"Wrong connection");
    level.connectionStates[44]=0x13c;check(doorFade(level,16)==100,"Door fade boundary");
    level.connectionStates[44]=0x11e;check(doorFade(level,16)==50,"Door fade midpoint");
    level.connectionStates[44]=0x13d;
    SurvivalSession survival;ItemSession items;items.load(level);
    survival.vitals().applyDamage(123);survival.vitals().update(1,true);
    std::uint16_t card=0x5005;survival.inventory().takeSupply(card);
#ifdef EXTREMIS_DEBUG_MENU
    items.debugCompleteSecurities();items.updateSelfDestruction(123,true);
#endif
    const auto health=survival.vitals().health(),oxygen=survival.vitals().oxygen();
    const auto cards=survival.inventory().cards();const auto count=items.completedSecurities();
    const auto remaining=items.selfDestruction().remaining();const auto states=level.connectionStates;
    check(trigger(level,16) && level.connectionStates[44]==0,"Trigger did not close door");
    check(!trigger(level,16),"Duplicate activation");
    full(fixture());choicesAndEscape();
    check(survival.vitals().health()==health && survival.vitals().oxygen()==oxygen &&
          survival.inventory().cards()==cards && items.completedSecurities()==count &&
          items.selfDestruction().remaining()==remaining,"Communication changed campaign");
    for(unsigned i=0;i<states.size();++i) if(i!=44) check(level.connectionStates[i]==states[i],"Unrelated connection changed");
    check(level.zoneIndex==21 && level.accessCode==123,"Campaign access changed");
    level.connectionStates[44]=doors::dos::update(doors::dos::opening,61);
    check(trigger(level,16),"New opening cannot replay communication");
}
void actualDoor(const std::filesystem::path& directory) {
    auto level=game_lab::loadLevel(directory.parent_path()/"GAME.LAB",21).level;
    const auto doors=doors::closedDoors(level);
    const auto world=collision::staticWorld(level);
    DoorControl control(world,doors,level);
    Camera camera{3300,1500,3*3.14159265358979323846/2};camera.sector=16;
    const auto now=DoorControl::Clock::now();control.beginFrame(true,now);
    bool fired=false;
    for(int step=1;step<=100 && !fired;++step) {
        const auto time=now+std::chrono::milliseconds(20*step);
        control.beginFrame(true,time);auto before=camera;camera.y-=3;
        control.constrainPlayer(before,camera);control.updateDoors(time);control.resetCounter(time);
        fired=trigger(level,camera.sector.value_or(~0u));
    }
    check(fired && camera.sector==16 && level.connectionStates[44]==0,"Real door movement failed to trigger");
}
}
int main(int argc,char** argv) {
    try {
        triggerAndCampaign();timingAndPixels();
        auto broken=fixture();broken.sprites.pop_back();
        try {Presentation p(std::move(broken));throw std::logic_error("Malformed resource accepted");} catch(const std::runtime_error&) {}
        if(argc>1) {actualDoor(argv[1]);full(loadResources(argv[1]));}
        std::cout<<"Communication trigger, YES/NO, seven pages, Escape, repeat, timing and campaign preservation passed\n";
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
