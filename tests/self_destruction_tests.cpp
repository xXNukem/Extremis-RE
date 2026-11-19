#include "extremis/gameplay/player/self_destruction.h"
#include "extremis/world/door_control.h"
#include "extremis/ui/game_messages.h"
#include <iostream>
#include <stdexcept>
using namespace std::chrono_literals;
void check(bool value,const char* why) {if(!value) throw std::runtime_error(why);}
int main() {
    try {
        SelfDestruction countdown;
        check(!countdown.update(65535,false) && countdown.remaining()==48000,"Inactive counter changed");
        check(!countdown.update(65535,true) && !countdown.active(),"Unarmed update activated event");
        countdown.activate();
        collision::World world;std::vector<doors::Door> doors;game_lab::Level level;
        const DoorControl::Time origin{};DoorControl control(world,doors,level,origin);
        check(!control.beginFrame(true,origin),"Entry not discarded");
        check(control.beginFrame(true,origin+1s),"Active frame missing");
        auto ticks=control.gameplayTicks(origin+1s);
        check(ticks==161,"Did not read original PIT word including reset-to-one");
        countdown.update(ticks,true);control.resetCounter(origin+1s);
        check(countdown.remaining()==47839,"Subtraction used seconds/phase instead of word");
        countdown.activate();check(countdown.remaining()==47839,"Repeated activation reset time");
        control.beginFrame(false,origin+2s);
        countdown.update(control.gameplayTicks(origin+50s),false);
        check(countdown.remaining()==47839,"Paused clock charged destruction time");
        check(!control.beginFrame(true,origin+100s),"Resume must discard inactive time");
        control.resetCounter(origin+100s);
        check(control.beginFrame(true,origin+101s),"Resume failed");
        countdown.update(control.gameplayTicks(origin+101s),true);
        check(countdown.remaining()==47678,"Resume caught up paused interval");
        SelfDestruction overflow;overflow.activate();check(overflow.update(65535,true) && overflow.remaining()==0,"Unsigned word not zero-extended");
        check(!overflow.update(1,true),"Second expiry notification");
        messages::Queue q;q.postDestruction(48000);
        check(q.visible()[0]=="ATTENTION:\r\n5M 00S UNTIL EXPLOSION!","Initial display");
        q.postDestruction(47999);
        check(q.visible()[0]=="ATTENTION:\r\n4M 59S UNTIL EXPLOSION!" && q.visible()[1].empty(),"Formatting or duplicate entries");
        auto copy=q;q.clear();check(!copy.visible()[0].empty(),"Message storage not owned");
        copy.postDestruction(159);check(copy.visible()[0]=="ATTENTION:\r\n0M 00S UNTIL EXPLOSION!","Truncation below one second");
        copy.update(60,false);check(!copy.visible()[0].empty(),"Pause expired message");
        copy.update(5,true);check(copy.visible()[0].empty(),"Original message lifetime lost");
        std::cout<<"Self-destruction PIT, pause/resume, expiry and message tests passed\n";
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
