#include "app/session/shuttle_session.h"

#include "extremis/cinematics/playback_result.h"
#include "extremis/gameplay/player/player_inventory.h"

namespace shuttle {
ItemSession::Travel arrival() { return {22,{1,320,300,0},std::nullopt}; }

void Session::answer(bool yes,const player::Inventory& inventory,const ItemSession& items) {
    if(step_==Step::MissingCard || step_==Step::Securities) { step_=Step::Refused; return; }
    if(step_!=Step::Boarding && step_!=Step::Start) return;
    if(!yes) { step_=Step::Refused; return; }
    if(step_==Step::Boarding) { step_=Step::Start; return; }
    const auto& cards=inventory.cards();
    if(cards[0]!=0x2005 && cards[1]!=0x2005 && cards[2]!=0x2005) step_=Step::MissingCard;
    else if(items.completedSecurities()!=16) step_=Step::Securities;
    else step_=Step::Flight;
}
std::optional<ItemSession::Travel> Session::complete(StartupCreditsResult result,ItemSession& items) {
    if(step_!=Step::Flight || result!=StartupCreditsResult::Continue) return {};
    items.finishEvacuation();step_=Step::Arrived;return arrival();
}

unsigned doorFade(const game_lab::Level& level,unsigned sector) {
    if(level.zoneIndex!=21 || sector!=38 || level.connectionStates.size()<=45) return 0;
    const auto word=level.connectionStates[45];
    const int phase=static_cast<std::int8_t>(word&255);
    return (word>>8) && phase>0 && phase<=60 ? unsigned(phase)*100/60 : 0;
}
bool trigger(game_lab::Level& level,unsigned sector) {
    if(level.zoneIndex!=21 || sector!=38 || level.connectionStates.size()<=45) return false;
    const auto word=level.connectionStates[45];
    if(!(word>>8) || static_cast<std::int8_t>(word&255)<=60) return false;
    level.connectionStates[45]=0;
    return true;
}
} // namespace shuttle

namespace shuttle {
Step Session::step() const
{
    return step_;
}
} // namespace shuttle
