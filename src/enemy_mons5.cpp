#include "enemy_system.h"
#include "enemy_navigation.h"
#include "enemy_profiles.h"
#include <algorithm>
#include <cmath>

namespace enemies {
namespace {
constexpr double pi=3.14159265358979323846;
int signedPhase(std::uint16_t phase) { return phase<0x8000 ? int(phase) : int(phase)-65536; }
void pose(game_lab::EnemyRecord& e, bool attack)
{
    // EXE 0xec08 / 0xf0cc: signed angular phase, not a sprite counter.
    const int phase=signedPhase(e.phase);
    const unsigned image=phase<-80 ? 0 : phase<-40 ? 1 : phase<0 ? 2 :
        attack ? (phase<60 ? 6 : phase<120 ? 7 : 8) :
        (phase<120 ? 3 : phase<240 ? 4 : phase<360 ? 3 : phase<400 ? 2 : 1);
    e.sprite=std::uint16_t(0x8000|(e.sprite&0x2000)|image);
    const int angle=std::clamp(phase,0,360);
    e.elevation=std::uint16_t((e.elevation&0xff00)|original::elevation5[angle<=180 ? angle : 360-angle]);
}
}
void System::displace5(std::size_t i, const game_lab::Level& level,
                       const collision::World& walls, double distance, bool crossSector)
{
    auto& e=records_[i];
    const collision::Point p{origins_[i].x+e.x,origins_[i].y+e.y};
    const double angle=e.angle*pi/360;
    auto moved=collision::move(walls,p,{distance*std::cos(angle),distance*std::sin(angle)});
    // Commit only a representable, physically located position, also while airborne.
    moved={std::round(moved.x),std::round(moved.y)};
    const auto next=collision::locate(level,moved,e.sector);
    if (!next || (!crossSector && next->sector!=e.sector)) return;
    const auto& sector=level.sectors[next->sector];
    e.sector=std::uint16_t(next->sector); e.polygon=std::uint16_t(next->polygon);
    origins_[i]={double(sector.gx),double(sector.gy)};
    e.x=std::uint16_t(moved.x-sector.gx); e.y=std::uint16_t(moved.y-sector.gy);
}
void System::advance5(std::size_t i, const game_lab::Level& level, const Camera& camera,
                      const collision::World& walls, std::optional<collision::Location> playerLocation)
{
    auto& e=records_[i];
    const collision::Point p{origins_[i].x+e.x,origins_[i].y+e.y};
    const auto location=collision::locate(level,p,e.sector);
    e.polygon=location ? std::uint16_t(location->polygon) : 0xffff;
    // EXE 0xf1bd: the high byte is an invalid-polygon timeout, not altitude.
    const unsigned missing=location ? 0 : (e.elevation>>8)+1;
    e.elevation=std::uint16_t((e.elevation&255)|(missing<<8));
    if (missing>=100) { e.health=0xffff; return; }
    // EXE 0xea34..0xebcc: interrupt the jump, halve height, then land on recovery.
    if (e.state==4 && e.phase>=1) {
        e.state=0; e.phase=0; e.elevation&=0xff00;
    } else if (e.state==4 || e.state==5) {
        const bool dying=e.state==5;
        if (!dying) ++e.phase;
        e.sprite=std::uint16_t(0x8000|(dying ? 0 : e.sprite&0x2000)|((e.phase&1)?9:5));
        e.elevation=std::uint16_t((e.elevation&255)/2);
        if (e.behavior) displace5(i,level,walls,-double(recoil5_[i]),true);
        if (dying && ++e.phase>=8) {
            e.health=0xffff; sounds_.push_back(17);
            effects_.begin({origins_[i].x+e.x,origins_[i].y+e.y},e.elevation&255,
                           camera,baseHeight_,true,bank_);
        }
        return;
    }
    const auto attackStep=[&]() {
        // 0xebdb..0xed0c: no horizontal travel during the attack.
        const int phase=signedPhase(e.phase)+40;
        e.phase=std::uint16_t(phase);
        if (phase>=360) {
            e.state=e.phase=std::uint16_t(-60); // Native landing sentinel.
            e.elevation&=0xff00;
            return false;
        }
        pose(e,true);
        if (phase>60 && playerLocation && playerLocation->sector==e.sector) {
            reaction_=8; sounds_.push_back(9+(attackSound_++&1));
        }
        return true;
    };
    if (e.state==2 && attackStep()) return;

    const bool pursuing=playerLocation &&
        (e.behavior>=3 || playerLocation->sector==e.sector);
    std::optional<collision::Point> destination;
    std::optional<collision::Location> homeLocation;
    if (location && (pursuing || e.behavior)) {
        if (pursuing) destination=waypoint(level,*location,p,*playerLocation,{camera.x,camera.y});
        else {
            const collision::Point home{origins_[i].x+e.homeX,origins_[i].y+e.homeY};
            homeLocation=collision::locate(level,home,e.sector);
            if (homeLocation) destination=waypoint(level,*location,p,*homeLocation,home);
        }
    }
    bool travel=false;
    if (destination) {
        const double dx=destination->x-p.x,dy=destination->y-p.y;
        const bool same=playerLocation && location->sector==playerLocation->sector &&
                        location->polygon==playerLocation->polygon;
        const bool close=std::max(std::abs(dx),std::abs(dy))<100 &&
            (pursuing ? same : (homeLocation && homeLocation->sector==location->sector &&
                               homeLocation->polygon==location->polygon));
        if (close && pursuing && same) {
            // 0xef0a really reads sprite (+0x14), including its signed flag.
            // Preserve that native gate rather than substituting the jump phase.
            const int image=signedPhase(e.sprite);
            if (image<0 || image>=360) {
                if (image>=360) e.sprite=std::uint16_t(image-(image<400?200:240));
                e.state=2; e.sprite=(e.sprite&0x2000)^0x2000;
                if (!attackStep()) {
                    // Landing re-enters navigation immediately in the DOS routine.
                    e.state=2; e.sprite=(e.sprite&0x2000)^0x2000;
                    attackStep();
                }
                return;
            }
        } else if (!close && e.behavior>=1) {
            const double turn=std::remainder(std::atan2(dy,dx)*360/pi-e.angle,720);
            if (std::abs(turn)>1e-6) {
                const double amount=std::max(4.0,std::floor(std::abs(turn)/2));
                e.angle=std::uint16_t((int(std::lround(e.angle+std::copysign(amount,turn)))+720)%720);
            }
            // f3c5 tests the HALF turn against 180; it is not the common port gate.
            travel=true;
        }
    }
    if (!travel && !(e.elevation&255)) {
        e.state=6; e.phase=(e.phase+1)&31;
        if (!e.phase) e.sprite^=0x2000;
        e.sprite=std::uint16_t(0x8000|(e.sprite&0x2000)|original::waiting5[e.phase]);
        return;
    }
    if (travel) {
        const int phase=signedPhase(e.phase);
        displace5(i,level,walls,phase>=0 && phase<360 ? speed_ : speed_/2,
                  e.behavior>=3 && (!playerLocation || e.sector!=playerLocation->sector));
    }
    // 0xf062..0xf156 advances even if collision prevented horizontal displacement.
    int phase=signedPhase(e.phase)+
        (std::abs(std::remainder(camera.angle*360/pi-e.angle,720))<180 ? -40 : 40);
    if (phase<-120) { phase+=560; e.sprite^=0x2000; }
    else if (phase>=440) { phase-=560; e.sprite^=0x2000; }
    e.phase=std::uint16_t(phase); pose(e,false); e.state=0;
}
} // namespace enemies
