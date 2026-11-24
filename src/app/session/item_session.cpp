#include "app/session/item_session.h"
#include "extremis/world/original_travel.h"
#include "extremis/gameplay/player/survival_session.h"
#include "extremis/audio/game_audio.h"
#include "extremis/world/door_control.h"
#include <SDL3/SDL_log.h>
#include <algorithm>
#include <utility>
#include <stdexcept>

std::optional<ItemSession::Travel> ItemSession::takeTravel() { return std::exchange(travel_,std::nullopt); }

bool ItemSession::takeBombPlacement() { return std::exchange(bombPlacement_, false); }

void ItemSession::storePanelContents(game_lab::Level& level) const
{
    if (world_ && level.zoneIndex == zone_) world_->storePanelContents(level);
}

void ItemSession::load(const game_lab::Level& level)
{
    if (level.zoneIndex>=plans_.size()) throw std::runtime_error("Invalid map zone");
    animationTime_=planTicks_=0; resumeAnimation_=false; pendingCard_.reset();
    zone_=level.zoneIndex; travel_.reset(); bombPlacement_=false;
    world_.emplace(level); accessCode_=level.accessCode; authorized_=false;
    codeTargets_=level.codeTargets;
    interface_.reset(); panel_.reset(); take_=injector_=oxygen_=false; action_=Action::None;
    inventoryRequested_=false; keysDown_.fill(false);
    SDL_Log("I: toggle inventory; E: interact/confirm; WASD/arrows: navigate; Z: heal; X: oxygen; Escape: port menu; Backspace: close interface.");
}
bool ItemSession::modal() const { return interface_.has_value(); }
bool ItemSession::planTransferred() const { return planTransferred(zone_); }
bool ItemSession::planTransferred(unsigned zone) const { return zone<plans_.size() && plans_[zone]; }
void ItemSession::resetPlans() { plans_.fill(false); securities_.fill(false); selfDestruction_=SelfDestruction{}; completedSecurities_=0; }
bool ItemSession::securitiesComplete() const {
    return std::all_of(securities_.begin(),securities_.end(),[](bool disabled){return disabled;});
}
bool ItemSession::updateSelfDestruction(std::uint16_t ticks,bool simulationActive) {
    // Once armed in level22, the original event persists across level loads.
    return selfDestruction_.update(ticks,simulationActive && !busy());
}
#ifdef EXTREMIS_DEBUG_MENU
void ItemSession::debugCompleteSecurities() {
    if(zone_==21) { securities_.fill(true);completedSecurities_=16;selfDestruction_.activate(); }
}
void ItemSession::debugShortenDestruction() {
    if(securitiesComplete()) selfDestruction_.debugShorten();
}
#endif
void ItemSession::clearInput()
{
    action_=Action::None; inventoryRequested_=take_=injector_=oxygen_=false;
    keysDown_.fill(false);
}
const interaction::State* ItemSession::interfaceState() const { return interface_?&*interface_:nullptr; }
void ItemSession::setAvailable(bool available)
{
    available_=available;
    if (!available) { interface_.reset(); inventoryRequested_=take_=injector_=oxygen_=false; action_=Action::None; }
}

void ItemSession::open(std::size_t index, SurvivalSession& player)
{
    player.clearMessages();
    panel_=index;
    interface_.emplace(); blink_=0;
    const auto& panel=world_->panels()[index];
    interface_->contents=panel.contents;
    interface_->zoneName=interaction::original::codes[zone_].zone;
    interface_->securityPanel=panel.securityPanel;
    interface_->codeReader=panel.codeReader;
    interface_->elevator=panel.elevator;
    interface_->elevatorIndex=panel.contents;
    interface_->elevatorZone=zone_;
    interface_->elevatorSector=unsigned(panel.sector);
    if (panel.elevator || panel.cardReader || panel.codeReader || panel.securityPanel) showContents(player);
    else {
        interface_->screen=interaction::Screen::Code;
        interface_->codeAccepted=authorized_;
        if(authorized_) interface_->notice="CODE ZONE "+interface_->zoneName+" ALREADY SEIZED";
    }
    startPhase(interaction::Phase::Opening);
}
void ItemSession::showContents(const SurvivalSession& player)
{
    auto& s=*interface_; s.selection=0; s.notice.clear();
    const auto& panel=world_->panels()[*panel_];
    if (panel.codePanel) s.screen=interaction::Screen::Plan;
    else if (panel.elevator || panel.cardReader || panel.codeReader || panel.securityPanel) {
        s.screen=interaction::Screen::Cards;
        s.cards=player.inventory().cards();
        // Availability is not compatibility. Show even one incompatible card;
        // the reader rejects its family/identifier only when confirmed.
        if (std::none_of(s.cards.begin(),s.cards.end(),[](int c){return c>=0;}))
            s.notice=panel.elevator?"No elevator card":"No security card";
    } else s.screen=panel.weapon?interaction::Screen::Weapon:interaction::Screen::Supply;
}
void ItemSession::navigate(Action action)
{
    auto& s=*interface_;
    const bool horizontal=action==Action::Left || action==Action::Right;
    if (s.screen==interaction::Screen::Elevator) {
        if (!horizontal) s.selection=unsigned(std::clamp(int(s.selection)+(action==Action::Up?1:-1),0,3));
        s.notice.clear(); return;
    }
    if (s.screen==interaction::Screen::Inventory) {
        if (horizontal) return;
        const int delta=action==Action::Up?-1:1;
        if (s.inventoryUse) {
            const int last=s.selection==0 || (s.selection==2 && s.injectors[0]>0 && s.injectors[1]>0) ? 2 : 1;
            s.useSelection=unsigned(std::clamp(int(s.useSelection)+delta,0,last));
        }
        else s.selection=unsigned(std::clamp(int(s.selection)+delta,0,5));
        blink_=0; s.blinkVisible=true; return;
    }
    if ((s.screen==interaction::Screen::Cards || s.screen==interaction::Screen::Codes) && horizontal) return;
    if ((s.screen==interaction::Screen::Supply || s.screen==interaction::Screen::Weapon) &&
        !horizontal && !s.replacing) return;
    if (s.screen!=interaction::Screen::Codes) s.notice.clear();
    const int delta=(action==Action::Left || action==Action::Up)?-1:1;
    if (s.screen==interaction::Screen::Code) {
        const int row=int(s.selection)/7, col=int(s.selection)%7;
        if (action==Action::Left || action==Action::Right)
            s.selection=unsigned(row*7+std::clamp(col+delta,0,6));
        else s.selection=unsigned(std::clamp(row+delta,0,1)*7+col);
    } else if (s.screen==interaction::Screen::Cards) {
        unsigned count=unsigned(std::count_if(s.cards.begin(),s.cards.end(),[](int c){return c>=0;}));
        s.selection=unsigned(std::clamp(int(s.selection)+delta,0,int(count)));
    } else s.selection=unsigned(std::clamp(int(s.selection)+delta,0,1));
    blink_=0; s.blinkVisible=true;
}
void ItemSession::confirmCode(SurvivalSession& player)
{
    auto& s=*interface_;
    s.pressedKey=s.selection; s.fingerPhase=1;
    if (s.selection==6) { if (!s.entered.empty()) s.entered.pop_back(); return; }
    if (s.selection!=13) {
        if (s.entered.size()<6) s.entered+=char('A'+s.selection-(s.selection>6?1:0));
        return;
    }
    const auto checksum=interaction::codeChecksum(s.entered);
    if (!checksum) return;
    if (!accessCode_) { s.notice="CODE DATA UNAVAILABLE"; return; }
    s.codeAccepted=*checksum==*accessCode_;
    s.notice=s.codeAccepted?"OK...":"WRONG CODE!!...";
    startPhase(interaction::Phase::CodeResult);
    (void)player;
}
void ItemSession::confirmCard(SurvivalSession& player, GameAudio& audio, DoorControl* security)
{
    auto& s=*interface_;
    unsigned index=0;
    std::optional<std::uint16_t> card;
    for (int value:s.cards) if (value>=0) {
        if (index++==s.selection) card=std::uint16_t(value);
    }
    if (!card) { if (s.elevator) audio.interfaceMotor(); interface_.reset(); return; } // Original Exit entry.
    if (s.replacing) {
        const auto result=world_->replaceCard(*panel_,player.inventory(),s.selection);
        if (result==player::Transfer::Exchanged) { s.screen=interaction::Screen::Supply; s.contentTaken=true; closePanel(audio); }
        return;
    }
    if (s.elevator) {
        if ((*card&0xf000)!=0) { s.notice="Incompatible card type"; return; }
        audio.interfaceMotor();
        s.screen=interaction::Screen::Elevator; s.selection=0;
        s.useSelection=*card&0xff; s.notice.clear(); pendingCard_=card;
        startPhase(interaction::Phase::Opening); return;
    }
    if ((*card&0xf000)!=(s.securityPanel?0x2000:0x1000)) { s.notice="Incompatible card type"; return; }
    pendingCard_=card;
    s.screen=interaction::Screen::Reader; s.notice.clear();
    startPhase(interaction::Phase::Opening);
    audio.interfaceMotor();
}
void ItemSession::usePendingCard(SurvivalSession& player, GameAudio& audio, DoorControl* security)
{
    auto& s=*interface_;
    if (!pendingCard_) return;
    if (s.codeReader) {
        if ((*pendingCard_&0xff)!=(s.contents&0xff)) {
            s.screen=interaction::Screen::Reader; s.notice="CARD REFUSED...";
        } else {
            audio.acceptCard(); s.cardAccepted=true;
            s.notice="CARD ACCEPTED...."; s.selection=0; s.codeIndex=0;
        }
        return;
    }
    if (s.securityPanel) {
        const unsigned id=s.contents&0xff;
        // EXE 0x95e4: global byte[0x60a7 + id - 1]; never touches a door.
        s.cardAccepted=id>=1 && id<=securities_.size() &&
            (*pendingCard_&0xff)==(s.contents>>8);
        if (s.cardAccepted) {
            if(!securities_[id-1]) ++completedSecurities_;
            securities_[id-1]=true; audio.acceptCard();
            if(zone_==21 && completedSecurities_==16) selfDestruction_.activate();
            s.notice="SECURITY "+std::to_string(id)+" DEACTIVATED..";
        } else s.notice="IMPOSSIBLE TO DEACTIVATE THE SECURITY";
        return;
    }
    if (!security) { interface_.reset(); return; }
    const auto result=security->useCardReader(s.contents,pendingCard_);
    using Result=DoorControl::CardResult;
    if (result==Result::Unlocked) { audio.acceptCard(); player.postMessage(messages::Id::DoorUnlocked); }
    else if (result==Result::AlreadyDisabled) player.postMessage(messages::Id::AlreadyDisabled);
    else player.postMessage(messages::Id::CardRejected);
    s.screen=interaction::Screen::Reader;
    s.notice=std::string(messages::text(result==Result::Unlocked?messages::Id::DoorUnlocked:
        result==Result::AlreadyDisabled?messages::Id::AlreadyDisabled:messages::Id::CardRejected));
}

void ItemSession::showCode()
{
    auto& s=*interface_;
    if (s.codeIndex>=codeTargets_.size()) { s.notice="CODE DATA UNAVAILABLE"; return; }
    const auto index=codeTargets_[s.codeIndex];
    if (index>=interaction::original::codes.size()) { s.notice="CODE DATA UNAVAILABLE"; return; }
    const auto& code=interaction::original::codes[index];
    s.notice="CODE ZONE "+std::string(code.zone)+"  "+std::string(code.letters);
}
void ItemSession::confirm(SurvivalSession& player, GameAudio& audio, DoorControl* security)
{
    using Screen=interaction::Screen;
    switch(interface_->screen) {
    case Screen::Elevator: {
        const auto& s=*interface_;
        const auto* route=interaction::original::route(s.elevatorIndex,s.selection);
        if (!route || !interaction::original::admitted(*route,s.useSelection,
                                                     s.elevatorZone,s.elevatorSector)) return;
        const auto& lift=interaction::original::lifts[s.elevatorIndex];
        if(route->zone!=s.elevatorZone || route->sector!=s.elevatorSector) {
            const auto source=std::find_if(lift.routes.begin(),lift.routes.end(),[&](const auto& r) {
                return r.exists && r.zone==s.elevatorZone && r.sector==s.elevatorSector;
            });
            if(source==lift.routes.end()) return; // Invalid source: never invent a floor.
            travel_=Travel{route->zone,{route->sector,lift.x,lift.y,lift.angle},
                           ElevatorTravel{s.elevatorIndex,source->floor,route->floor}};
        }
        audio.interfaceKey(); interface_.reset(); clearInput();
        return;
    }
    case Screen::Inventory:
        audio.interfaceKey();
        if (interface_->selection==5 || (interface_->selection==0 && interface_->inventoryUse))
            audio.interfaceMotor();
        confirmInventory(player); return;
    case Screen::Codes:
        audio.interfaceKey();
        if (interface_->selection) { interface_->screen=Screen::Reader; interface_->notice.clear(); startPhase(interaction::Phase::CardWithdraw); }
        else if (!codeTargets_.empty()) {
            interface_->codeIndex=(interface_->codeIndex+1)%codeTargets_.size(); showCode();
        }
        return;
    case Screen::Code: audio.interfaceKey(); confirmCode(player); return;
    case Screen::Cards: audio.interfaceKey(); confirmCard(player,audio,security); return;
    case Screen::Reader: startPhase(interaction::Phase::CardWithdraw); return;
    case Screen::Plan: return;
    case Screen::Supply: case Screen::Weapon: break;
    }
    audio.confirmItemSelection(); // Original confirmation precedes both Yes and No branches.
    if (interface_->selection) { closePanel(audio); return; }
    const auto result=world_->take(*panel_,player.inventory(),interface_->replacing);
    if (result==player::Transfer::Taken || result==player::Transfer::Exchanged) {
        interface_->contentTaken=true;
        closePanel(audio);
    } else if (result==player::Transfer::Empty) {
        player.postMessage(messages::Id::EmptyPanel); interface_.reset();
    } else if ((interface_->contents>>12)>=3 && (interface_->contents>>12)<=5) {
        interface_->screen=Screen::Cards; interface_->cards=player.inventory().cards();
        interface_->selection=0; interface_->replacing=true;
    } else if ((interface_->contents>>12)==6) interface_->notice="ALREADY HAVE A MINE";
    else if (result==player::Transfer::Capacity && interface_->screen==Screen::Supply &&
             (interface_->contents>>12)==0) {
        interface_->replacing=true;
        interface_->batteryCharge=player.inventory().batteryCharge();
        interface_->selection=0;
        blink_=0;
    } else interface_->notice=(interface_->contents>>12)==1?
        "ALREADY HAVE AN OXYGEN BOTTLE":"ALREADY HAVE TWO INJECTORS";
}
void ItemSession::update(const Camera* camera, bool active, const collision::World& obstacles,
                         SurvivalSession& player, GameAudio& audio, DoorControl* security, double seconds,
                         bool enemyInConnectedSet)
{
    const bool inventory=std::exchange(inventoryRequested_,false);
    const bool take=std::exchange(take_,false), injector=std::exchange(injector_,false);
    const bool oxygen=std::exchange(oxygen_,false);
    const auto action=std::exchange(action_,Action::None);
    if (!player.alive()) { interface_.reset(); return; }
    if (modal() && action==Action::Cancel) {
        if (interface_->screen==interaction::Screen::Inventory || interface_->elevator) audio.interfaceMotor();
        interface_.reset(); return;
    }
    if (!camera || !active) { resumeAnimation_=true; return; }
    if (resumeAnimation_) { seconds=0; resumeAnimation_=false; }
    if (modal()) {
        if (interface_->pressedKey && ++interface_->fingerPhase>3) {
            interface_->pressedKey.reset(); interface_->fingerPhase=0;
        }
        if (animate(seconds,player,audio,security)) return;
        interface_->blinkVisible=(blink_++%64)<32; // Original selector counter, not a recovered Hz rate.
        if (action==Action::Confirm) confirm(player,audio,security);
        else if (action!=Action::None) {
            const auto before=interface_->selection;
            navigate(action);
            if (interface_->screen==interaction::Screen::Cards && interface_->selection!=before)
                audio.interfaceKey();
        }
        return;
    }
    if (inventory && available_ && !enemyInConnectedSet) {
        player.clearMessages(); // EXE 0x6f17 clears the queue before inventory mode 0.
        panel_.reset(); interface_.emplace(); blink_=0;
        interface_->screen=interaction::Screen::Inventory;
        refreshInventory(player); audio.interfaceMotor(); return;
    }
    if (take) {
        const auto target=world_?world_->target(*camera,obstacles):std::nullopt;
        if (target) {
            if (!available_) return; // Never open an invisible interface or activate a door behind it.
            const auto& panel=world_->panels()[*target];
            if (panel.securityPanel && (panel.contents&0xff)>=1 &&
                (panel.contents&0xff)<=securities_.size() && securities_[(panel.contents&0xff)-1]) {
                player.postMessage(messages::Id::AlreadyDisabled); return;
            }
            if (panel.cardReader && security &&
                security->useCardReader(panel.contents,{})==DoorControl::CardResult::AlreadyDisabled) {
                player.postMessage(messages::Id::AlreadyDisabled);
                return;
            }
            if (!panel.codePanel && !panel.cardReader && !panel.codeReader && !panel.elevator && !panel.securityPanel && panel.contents==0xffff)
                player.postMessage(messages::Id::EmptyPanel);
            else {
                if (panel.elevator) audio.interfaceMotor();
                open(*target,player);
                if (interface_->screen!=interaction::Screen::Cards) audio.interfaceMotor();
            }
        }
        return; // One E selects one context; opening never confirms or also uses a door.
    }
    if (injector) player.inventory().useInjector(player.vitals());
    if (oxygen) player.inventory().useOxygen(player.vitals());
}

void ItemSession::refreshInventory(const SurvivalSession& player)
{
    auto& s=*interface_;
    s.cards=player.inventory().cards(); s.injectors=player.inventory().injectors();
    s.oxygenReserve=player.inventory().oxygenReserve(); s.mine=player.inventory().mine();
    s.batteryCharge=player.inventory().batteryCharge(); s.batteryMode=player.inventory().batteryMode();
}
void ItemSession::confirmInventory(SurvivalSession& player)
{
    auto& s=*interface_;
    if (s.selection==5) { interface_.reset(); return; }
    if (s.inventoryUse) {
        if (s.selection==4 && !s.useSelection && player.inventory().useMine()) {
            bombPlacement_=true;
            interface_.reset();
            return;
        }
        if (s.selection==0) {
            using Mode=player::BatteryMode;
            if (s.useSelection==0)
                player.inventory().setBatteryMode(s.batteryMode==Mode::Off ? Mode::Radar : Mode::Off);
            else if (s.useSelection==1)
                player.inventory().setBatteryMode(s.batteryMode==Mode::Optics ? Mode::Radar : Mode::Optics);
            interface_.reset();
            return;
        } else if (s.selection==2 && s.injectors[0]>0 && s.injectors[1]>0) {
            if (s.useSelection<2) player.inventory().useInjector(player.vitals(),s.useSelection);
        } else if (!s.useSelection) {
            if (s.selection==1) player.inventory().useOxygen(player.vitals());
            else if (s.selection==2) player.inventory().useInjector(player.vitals());
        }
        s.inventoryUse=false; refreshInventory(player); return;
    }
    if ((s.selection==4 && s.mine>=0) || (s.selection==0 && s.batteryCharge>0) || (s.selection==1 && s.oxygenReserve>0) ||
        (s.selection==2 && std::any_of(s.injectors.begin(),s.injectors.end(),[](int i){return i>0;}))) {
        s.inventoryUse=true; s.useSelection=0;
    }
}

unsigned ItemSession::completedSecurities() const
{
    return completedSecurities_;
}

void ItemSession::finishEvacuation()
{
    completedSecurities_=0;
    selfDestruction_.deactivate();
}

const SelfDestruction& ItemSession::selfDestruction() const
{
    return selfDestruction_;
}

bool ItemSession::busy() const
{
    return modal() || inventoryRequested_ || take_;
}
