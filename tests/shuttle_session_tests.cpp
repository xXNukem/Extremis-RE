#include "app/session/shuttle_session.h"
#include "app/startup/startup_credits.h"
#include "extremis/gameplay/player/player_inventory.h"
#include <iostream>
#include "extremis/world/door_control.h"
#include "extremis/world/door_clock.h"
#include "extremis/world/original_travel.h"
#include <SDL3/SDL.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <stdexcept>
using shuttle::Step;
void check(bool b,const char* what) {if(!b) throw std::runtime_error(what);}
int main(int argc,char** argv) {
 try {
    game_lab::Level level{};level.zoneIndex=21;level.connectionStates.resize(46);
    for(unsigned word:{0u,0x100u,0x13cu,0x3du,0x180u,0x1ffu}) {
        level.connectionStates[45]=word;check(!shuttle::trigger(level,38),"premature trigger");
    }
    level.connectionStates[45]=0x13d;
    check(!shuttle::trigger(level,37),"wrong sector");
    level.zoneIndex=20;check(!shuttle::trigger(level,38),"wrong zone");level.zoneIndex=21;
    check(shuttle::trigger(level,38) && level.connectionStates[45]==0,"door must close unlocked");
    check(!shuttle::trigger(level,38),"duplicate trigger");
    level.connectionStates[45]=0x13d;check(shuttle::trigger(level,38),"retry");
    level.connectionStates[45]=0x13c;check(shuttle::doorFade(level,38)==100,"door fade");
    ItemSession items;items.load(level);player::Inventory inventory;
    shuttle::Session no;no.answer(false,inventory,items);check(no.step()==Step::Refused,"boarding NO");
    shuttle::Session second;second.answer(true,inventory,items);check(second.step()==Step::Start,"boarding YES");
    second.answer(false,inventory,items);check(second.step()==Step::Refused,"start NO");
    shuttle::Session missing;missing.answer(true,inventory,items);missing.answer(true,inventory,items);
    check(missing.step()==Step::MissingCard,"card priority");missing.answer(true,inventory,items);
    check(missing.step()==Step::Refused,"error acknowledgement");
    std::uint16_t card=0x5008;inventory.takeSupply(card);
    shuttle::Session wrong;wrong.answer(true,inventory,items);wrong.answer(true,inventory,items);
    check(wrong.step()==Step::MissingCard,"exact card");
    card=0x5005;inventory.takeSupply(card);
    shuttle::Session off;off.answer(true,inventory,items);off.answer(true,inventory,items);
    check(off.step()==Step::Securities,"security requirement");
#ifdef EXTREMIS_DEBUG_MENU
    items.debugCompleteSecurities();items.updateSelfDestruction(100,true);
    const auto remainder=items.selfDestruction().remaining();const auto cards=inventory.cards();
    shuttle::Session success;success.answer(true,inventory,items);success.answer(true,inventory,items);
    check(success.step()==Step::Flight,"successful departure");
    check(items.completedSecurities()==16 && items.selfDestruction().active(),"disabled before movie completion");
    items.updateSelfDestruction(50000,false);check(items.selfDestruction().remaining()==remainder,"modal timer");
    check(!success.complete(StartupCreditsResult::Error,items) && items.completedSecurities()==16,"failed movie arrival");
    check(!success.complete(StartupCreditsResult::Quit,items) && items.selfDestruction().active(),"quit movie arrival");
    auto completed=success.complete(StartupCreditsResult::Continue,items);check(bool(completed),"completed arrival");
    auto destination=*completed;
    check(!success.complete(StartupCreditsResult::Continue,items),"duplicate arrival");
    check(destination.zone==22 && destination.arrival.sector==1 && destination.arrival.x==320 &&
          destination.arrival.y==300 && destination.arrival.angle==0 && shuttle::arrivalFloor==8 && !destination.elevator,"arrival");
    level.zoneIndex=22;items.load(level);
    check(items.completedSecurities()==0 && items.securitiesComplete() && !items.selfDestruction().active(),"arrival state");
    items.updateSelfDestruction(50000,true);
    check(items.selfDestruction().remaining()==remainder && inventory.cards()==cards,"retained state");
#endif
    check(shuttle::refusal.sector==38 && shuttle::refusal.x==300 && shuttle::refusal.y==900 && shuttle::refusal.angle==0,"refusal location");
    const auto& base=interaction::original::lifts[19].routes[0];
    check(base.zone==22 && base.sector==1 && base.floor==shuttle::arrivalFloor,"existing floor route");
    if(argc==2) {
        const std::filesystem::path directory=argv[1];
        auto actual=game_lab::loadLevel(directory.parent_path()/"GAME.LAB",21).level;
        const auto walls=collision::staticWorld(actual);const auto doors=doors::closedDoors(actual);
        DoorControl control(walls,doors,actual);
        const auto& sector=actual.sectors.at(38);
        Camera before;before.x=sector.gx+300;before.y=sector.gy+900;before.angle=3.14159265358979323846/2;before.sector=38;
        auto after=before;after.y+=30;control.constrainPlayer(before,after);
        check((actual.connectionStates[45]&0x100)!=0,"real door approach");
        for(unsigned i=0;i<61;++i) doors::dos::updateTable(actual.connectionStates,2);
        check(shuttle::trigger(actual,38),"real door phases");
        after=before;after.y+=30;control.constrainPlayer(before,after);
        check((actual.connectionStates[45]&0x100)!=0,"real retry approach");
        const auto dest=game_lab::loadLevel(directory.parent_path()/"GAME.LAB",22).level;
        check(dest.sectors.size()>1,"destination sector");
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER,"dummy");check(SDL_Init(SDL_INIT_VIDEO),"SDL init");
        auto* window=SDL_CreateWindow("boarding test",320,200,0);check(window,"window");
        auto* renderer=SDL_CreateRenderer(window,nullptr);check(renderer,"renderer");
        const auto id=SDL_GetWindowID(window);
        // Real assets and SDL input: NO, YES/NO, missing-card acknowledgement,
        // then successful eligibility. No gameplay callback executes in prompt().
        #ifdef EXTREMIS_DEBUG_MENU
        const unsigned scenarios=4;
#else
        const unsigned scenarios=3;
#endif
        for(unsigned scenario=0;scenario<scenarios;++scenario) {
            ItemSession state;state.load(actual);player::Inventory inv;
#ifdef EXTREMIS_DEBUG_MENU
            if(scenario==3) { state.debugCompleteSecurities();std::uint16_t c=0x5005;inv.takeSupply(c); }
#endif
            shuttle::Session dialog;
            std::atomic<bool> done=false;
            std::thread input([&] {
                const auto key=[&](SDL_Scancode code) {
                    SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;e.key.windowID=id;e.key.scancode=code;SDL_PushEvent(&e);
                    e.type=SDL_EVENT_KEY_UP;SDL_PushEvent(&e);
                };
                SDL_Event focus{};focus.type=SDL_EVENT_WINDOW_FOCUS_GAINED;focus.window.windowID=id;SDL_PushEvent(&focus);
                for(unsigned tick=0;tick<100 && !done;++tick) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    if(tick==5) { key(SDL_SCANCODE_ESCAPE);key(SDL_SCANCODE_I);key(SDL_SCANCODE_Z); }
                    if(tick==10) { if(scenario==0) key(SDL_SCANCODE_DOWN);key(SDL_SCANCODE_E); }
                    if(tick==30) { if(scenario==1) key(SDL_SCANCODE_DOWN);key(SDL_SCANCODE_E); }
                    if(tick==35) key(SDL_SCANCODE_E);
                }
                if(!done) { SDL_Event quit{};quit.type=SDL_EVENT_QUIT;SDL_PushEvent(&quit); }
            });
            const auto result=shuttle::prompt(window,renderer,directory,dialog,inv,state);
            done=true;input.join();
            check(result==StartupCreditsResult::Continue,"resource prompt completion");
            check(dialog.step()==(scenario==3?Step::Flight:Step::Refused),"SDL prompt route");
        }
        SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();
    }
    std::cout<<"Shuttle session tests passed\n";return 0;
 } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
