#include "item_session.h"
#include "survival_session.h"
#include "game_audio.h"
#include <algorithm>
#include <cmath>

namespace {
using interaction::Phase;
unsigned frames(Phase phase)
{
    switch (phase) {
    case Phase::Opening: case Phase::Closing: return 25; // 4..100, step 4.
    case Phase::CodeResult: return 70; // EXE 0x19fe7: 70 vertical retraces.
    case Phase::HandIn: return 16;
    case Phase::HandOut: return 19; // Hand y70..146, step 4.
    case Phase::PanelSlide: return 47; // -231..-47, step 4.
    case Phase::PanelZoom: return 26; // 100..50, step -2.
    case Phase::Shutters: return 22; // right leaf x145..229, step 4.
    case Phase::ContentExpand: return 49; // 4..100, step 2.
    case Phase::ContentMove: return 10; // 10..100, step 10.
    case Phase::ContentReveal: return 50; // 0..98, step 2.
    case Phase::CardInsert: return 21; // 0..100, step 5.
    case Phase::CardCheck: case Phase::CardResult: return 60;
    case Phase::CardWithdraw: return 10; // y52..132, step 8.
    case Phase::MovieIn: return 26;
    case Phase::Movie: return 73; // Five frames at 33 ticks / 160 Hz; handled below.
    case Phase::MovieOut: return 51;
    case Phase::Ready: return 0;
    }
    return 0;
}
}
void ItemSession::startPhase(interaction::Phase phase)
{
    interface_->phase=phase; interface_->frame=0; animationTime_=0;
}
void ItemSession::closePanel(GameAudio& audio)
{
    startPhase(Phase::Closing); audio.interfaceMotor();
}
void ItemSession::finishPhase(SurvivalSession& player, GameAudio& audio, DoorControl* security)
{
    auto& s=*interface_;
    switch(s.phase) {
    case Phase::Opening:
        if (s.screen==interaction::Screen::Reader || s.screen==interaction::Screen::Elevator) startPhase(Phase::CardInsert);
        else if (s.screen==interaction::Screen::Plan || s.screen==interaction::Screen::Supply ||
                 s.screen==interaction::Screen::Weapon) startPhase(Phase::PanelSlide);
        else if(s.screen==interaction::Screen::Code) startPhase(Phase::HandIn);
        else startPhase(Phase::Ready);
        break;
    case Phase::HandIn:
        if(s.screen==interaction::Screen::Codes) showCode();
        startPhase(s.codeAccepted?Phase::CodeResult:Phase::Ready); break;
    case Phase::CodeResult:
        if (s.codeAccepted) { authorized_=true; startPhase(Phase::HandOut); }
        else startPhase(Phase::Ready); // Keep the established retry/Escape controls.
        break;
    case Phase::HandOut: showContents(player); startPhase(Phase::PanelSlide); audio.interfaceMotor(); break;
    case Phase::PanelSlide: startPhase(Phase::PanelZoom); break;
    case Phase::PanelZoom:
        startPhase(Phase::Shutters); s.transfer=100; planTicks_=0;
        audio.interfaceShutters(); break;
    case Phase::Shutters:
        startPhase(s.screen==interaction::Screen::Plan?Phase::Ready:Phase::ContentExpand); break;
    case Phase::ContentExpand: startPhase(Phase::ContentMove); break;
    case Phase::ContentMove: startPhase(Phase::ContentReveal); break;
    case Phase::ContentReveal: startPhase(Phase::Ready); break;
    case Phase::CardInsert:
        if(s.elevator) startPhase(Phase::HandIn);
        else { s.notice=s.codeReader?"CONTROL PANEL....":"CONTROLLER CARD.."; startPhase(Phase::CardCheck); }
        audio.insertCard(); break;
    case Phase::CardCheck:
        usePendingCard(player,audio,security);
        if (interface_) startPhase(Phase::CardResult);
        break;
    case Phase::CardResult:
        s.notice.clear();
        if(s.codeReader && s.cardAccepted) { s.screen=interaction::Screen::Codes; startPhase(Phase::HandIn); }
        else startPhase(Phase::CardWithdraw);
        break;
    case Phase::CardWithdraw: pendingCard_.reset(); closePanel(audio); break;
    case Phase::Closing:
        if(s.contentTaken) { startPhase(Phase::MovieIn); audio.pickupTransfer(); }
        else { interface_.reset(); pendingCard_.reset(); }
        break;
    case Phase::MovieIn: startPhase(Phase::Movie); s.movieSeconds=33.0/160; break;
    case Phase::Movie: startPhase(Phase::MovieOut); break;
    case Phase::MovieOut: interface_.reset(); break;
    case Phase::Ready: break;
    }
}
bool ItemSession::animate(double seconds, SurvivalSession& player, GameAudio& audio, DoorControl* security)
{
    auto& s=*interface_;
    const bool timedPlan=s.screen==interaction::Screen::Plan &&
        (s.phase==Phase::Shutters || s.phase==Phase::Ready);
    if (!std::isfinite(seconds) || seconds<0) seconds=0;
    if (timedPlan) {
        // EXE 0x1a54b..0x1a564: strict >8 on the 160 Hz clock, decrement ONE.
        planTicks_+=seconds*160;
        const unsigned steps=unsigned(std::min(100.0,std::floor((planTicks_+1e-9)/9)));
        planTicks_-=steps*9; s.transfer-=std::min(steps,s.transfer);
        if (!s.transfer && s.phase==Phase::Ready) {
            plans_[zone_]=true; closePanel(audio); return true;
        }
    }
    if (s.phase==Phase::Ready) return timedPlan;
    if (s.phase==Phase::Movie) {
        s.movieSeconds+=seconds;
        if(s.movieSeconds>=5*33.0/160) finishPhase(player,audio,security);
        return true;
    }
    // Geometry loops have no recovered universal tick duration. A nominal VGA
    // 70 Hz presentation makes them independent of host FPS; see step 207.
    animationTime_+=seconds*70;
    unsigned count=frames(s.phase);
    if(s.phase==Phase::HandIn && s.elevator) count=19;
    if(s.phase==Phase::HandIn && s.screen==interaction::Screen::Codes) count=21;
    if(s.phase==Phase::CardCheck || s.phase==Phase::CardResult) count+=2*unsigned(s.notice.size());
    s.frame=unsigned(std::min(double(count-1),std::floor(animationTime_+1e-9)));
    if (animationTime_+1e-9>=count) finishPhase(player,audio,security);
    return true; // Input during a transition is consumed, never replayed.
}
