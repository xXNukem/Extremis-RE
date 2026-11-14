#include "door_control.h"
#include "motion_detector_tables.h"

#include <algorithm>
#include <cmath>

namespace {
// 0x3ee8: existing recovered atan table, 720 units per turn.
int wallAngle(int x, int y)
{
    const bool nx=x<0, ny=y<0;
    x=std::abs(x); y=std::abs(y);
    const bool swap=x<y;
    if (swap) std::swap(x,y);
    int a=detector::arctangent[x ? y*256/x : 0];
    if (swap) a=180-a;
    if (nx) a=360-a;
    if (ny) a=-a;
    return (a+720)%720;
}
int word(int value)
{
    const auto bits=static_cast<std::uint16_t>(value);
    return bits<0x8000 ? int(bits) : int(bits)-65536;
}
struct Local { int normal, tangent; };
Local rotate(int x, int y, int angle)
{
    const int c=detector::cosine[angle], s=detector::cosine[(angle+540)%720];
    // 0x1117a: high word after two shifts, including negative floor rounding.
    const auto high=[](std::int64_t value) {
        const auto shifted=value>=0 ? value/16384 : -((-value+16383)/16384);
        return word(int(shifted));
    };
    return {high(std::int64_t(x)*c-std::int64_t(y)*s),
            high(std::int64_t(y)*c+std::int64_t(x)*s)};
}
}

DoorControl::DoorControl(const collision::World& world, const std::vector<doors::Door>& doors,
                         game_lab::Level& level, Time now)
    : staticWorld_(world), doors_(doors), level_(level), counter_(now)
{
    resetLevel(now);
}
void DoorControl::resetLevel(Time now)
{
    counter_.inhibit(true,now); counter_.reset(); active_=false;
    rebuildWorld();
}
bool DoorControl::beginFrame(bool active, Time now)
{
    counter_.inhibit(!active,now);
    if (!active || !active_) {
        counter_.reset(); active_=active;
        return false; // Discard entry/resume intervals, preserving PIT phase.
    }
    return true;
}
void DoorControl::updateDoors(Time now)
{
    counter_.advance(now);
    if (active_) doors::dos::updateTable(level_.connectionStates,counter_.value());
    rebuildWorld();
}
void DoorControl::resetCounter(Time now)
{
    counter_.advance(now); counter_.reset();
}
void DoorControl::rebuildWorld()
{
    movementWorld_=staticWorld_;
    for (const auto& door:doors_) {
        if (door.stateIndex>=level_.connectionStates.size()) continue;
        if (!doors::dos::passageBranchEnabled(level_.connectionStates[door.stateIndex])) {
            const auto& f=door.faces[0];
            movementWorld_.push_back({{f.a.x,f.a.y},{f.b.x,f.b.y}});
        }
    }
}
DoorControl::CardResult DoorControl::useCardReader(std::uint16_t parameter,
                                                  std::optional<std::uint16_t> card)
{
    const auto index=parameter&0xff;
    if (index>=level_.connectionStates.size()) return CardResult::InvalidTarget;
    auto& state=level_.connectionStates[index];
    if (!(state&doors::dos::locked)) return CardResult::AlreadyDisabled;
    if (!card) return CardResult::NoCard;
    if ((*card&0xf000)!=0x1000 || (*card&0xff)!=(parameter>>8)) return CardResult::Rejected;
    state^=doors::dos::locked;
    return CardResult::Unlocked;
}

bool DoorControl::constrainPlayer(const Camera& before, Camera& proposed)
{
    // Global doubles remain the port's movement representation. Quantization is
    // only at this DOS boundary; untouched movement retains its fractional part.
    if (!std::isfinite(proposed.x) || !std::isfinite(proposed.y) || !std::isfinite(proposed.angle)) return false;
    if (!proposed.sector || *proposed.sector>=level_.sectors.size()) {
        const auto location=collision::locate(level_,{before.x,before.y},before.sector);
        if (!location) return false;
        proposed.sector=location->sector;
    }
    const auto& sector=level_.sectors[*proposed.sector];
    const int oldX=word(int(std::lround(before.x-sector.gx)));
    const int oldY=word(int(std::lround(before.y-sector.gy)));
    int x=word(int(std::lround(proposed.x-sector.gx)));
    int y=word(int(std::lround(proposed.y-sector.gy)));
    const int heading=(int(std::lround(std::remainder(proposed.angle,2*3.14159265358979323846)*360/3.14159265358979323846))+720)%720;
    bool locked=false;
    for (std::size_t i=0;i<sector.neighbors.size() && i<sector.walls.size();++i) {
        const auto& wall=sector.walls[i];
        if (wall.vertexA>=sector.vertices.size() || wall.vertexB>=sector.vertices.size() ||
            sector.neighbors[i]>=level_.sectors.size()) continue;
        const auto a=sector.vertices[wall.vertexA], b=sector.vertices[wall.vertexB];
        if (a.x==b.x && a.y==b.y) continue;
        const int angle=wallAngle(word(int(b.x)-a.x),word(int(b.y)-a.y));
        const int rotation=(180-angle+720)%720;
        const auto old=rotate(word(oldX-a.x),word(oldY-a.y),rotation);
        auto next=rotate(word(x-a.x),word(y-a.y),rotation);
        const auto found=std::find_if(doors_.begin(),doors_.end(),[&](const auto& door) {
            return door.stateIndex==wall.extra;
        });
        if (found==doors_.end() || (wall.type&0xf000)!=0x1000 || wall.extra>=level_.connectionStates.size()) {
            // Ordinary open connections keep existing movement, but own the
            // sector transition. No guessed automatic requests for special walls.
            const auto extent=rotate(word(int(b.x)-a.x),word(int(b.y)-a.y),rotation).tangent;
            if (old.normal>=0 && next.normal<0 && next.tangent>=0 && next.tangent<=extent) {
                proposed.sector=sector.neighbors[i]; break;
            }
            continue;
        }
        // 0x107db..0x10823: wall endpoints within proposed-position bounds.
        if (int(a.x)<x-300 || int(b.x)<x-300 || int(a.x)>=x+301 || int(b.x)>=x+301 ||
            int(a.y)<y-300 || int(b.y)<y-300 || int(a.y)>=y+301 || int(b.y)>=y+301) continue;
        if (std::min(old.normal,next.normal)>180 || std::max(old.normal,next.normal)<-180 ||
            std::min(old.tangent,next.tangent)>200 || std::max(old.tangent,next.tangent)<0) continue;
        auto& state=level_.connectionStates[wall.extra];
        const int relative=(heading-angle+1080)%720-360;
        const bool facing=relative>=120 && relative<240;
        if (!(wall.type&0x0800) && facing) {
            if (state&doors::dos::locked) locked=true;
            else state|=doors::dos::opening;
        }
        bool corrected=false, crossed=false;
        if (doors::dos::passageBranchEnabled(state)) {
            crossed=next.normal<0;
            if (next.normal>=-180) state=(state&doors::dos::locked)|0x0140;
            if (next.tangent<80) next.tangent=82;
            if (next.tangent>120) next.tangent=118;
            corrected=true;
        } else if (next.normal>=0) {
            if (state&0x01ff) {
                next.tangent+=next.tangent<100 ? std::min(10,100-next.tangent) : -std::min(10,next.tangent-100);
                next.normal=180; corrected=true;
            } else if (old.normal>=180 && next.normal<180) {
                // Closed/unrequested wall branch: intersect its clearance line.
                const int tangent=old.tangent+(next.tangent-old.tangent)*(180-old.normal)/(next.normal-old.normal);
                const int margin=(a.x==b.x || a.y==b.y)?80:30;
                const int extent=rotate(word(int(b.x)-a.x),word(int(b.y)-a.y),rotation).tangent;
                if (tangent>=-margin && tangent<=extent+margin) { next.normal=182; corrected=true; }
            }
        }
        if (corrected) {
            const auto result=rotate(next.normal,next.tangent,(720-rotation)%720);
            x=word(result.normal+a.x); y=word(result.tangent+a.y);
            proposed.x=double(sector.gx)+x; proposed.y=double(sector.gy)+y;
        }
        if (crossed) { proposed.sector=sector.neighbors[i]; break; }
    }
    return locked;
}
