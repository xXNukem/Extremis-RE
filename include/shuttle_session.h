#pragma once
#include "item_session.h"
#include "player_inventory.h"
#include "startup_credits.h"

namespace shuttle {
// Pure campaign decisions; presentation never owns inventory or countdown state.
enum class Step { Boarding, Start, MissingCard, Securities, Refused, Flight, Arrived };
inline ItemSession::Travel arrival() { return {22,{1,320,300,0},std::nullopt}; }
class Session {
public:
    Step step() const { return step_; }
    void answer(bool yes,const player::Inventory& inventory,const ItemSession& items) {
        if(step_==Step::MissingCard || step_==Step::Securities) { step_=Step::Refused; return; }
        if(step_!=Step::Boarding && step_!=Step::Start) return;
        if(!yes) { step_=Step::Refused; return; }
        if(step_==Step::Boarding) { step_=Step::Start; return; }
        const auto& cards=inventory.cards();
        if(cards[0]!=0x2005 && cards[1]!=0x2005 && cards[2]!=0x2005) step_=Step::MissingCard;
        else if(items.completedSecurities()!=16) step_=Step::Securities;
        else step_=Step::Flight;
    }
    std::optional<ItemSession::Travel> complete(StartupCreditsResult result,ItemSession& items) {
        if(step_!=Step::Flight || result!=StartupCreditsResult::Continue) return {};
        items.finishEvacuation();step_=Step::Arrived;return arrival();
    }
private:
    Step step_=Step::Boarding;
};
inline unsigned doorFade(const game_lab::Level& level,unsigned sector) {
    if(level.zoneIndex!=21 || sector!=38 || level.connectionStates.size()<=45) return 0;
    const auto word=level.connectionStates[45];
    const int phase=static_cast<std::int8_t>(word&255);
    return (word>>8) && phase>0 && phase<=60 ? unsigned(phase)*100/60 : 0;
}
inline bool trigger(game_lab::Level& level,unsigned sector) {
    if(level.zoneIndex!=21 || sector!=38 || level.connectionStates.size()<=45) return false;
    const auto word=level.connectionStates[45];
    if(!(word>>8) || static_cast<std::int8_t>(word&255)<=60) return false;
    level.connectionStates[45]=0;
    return true;
}
inline constexpr game_lab::PlayerStart refusal{38,300,900,0};
inline constexpr unsigned arrivalFloor=8;
// Continue + Flight means the caller can play the already validated movie.
StartupCreditsResult prompt(SDL_Window*,SDL_Renderer*,const std::filesystem::path&,
                            Session&,const player::Inventory&,const ItemSession&);
}
