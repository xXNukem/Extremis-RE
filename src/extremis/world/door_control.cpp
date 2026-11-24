#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>
#include "extremis/world/door_control.h"
#include "extremis/resources/sector_coordinates.h"
#include "vortex/core/math/constants.h"
#include "extremis/world/original_angle_tables.h"

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

bool nearDoor(Local old, Local next)
{
    return std::min(old.normal,next.normal)<=180 && std::max(old.normal,next.normal)>=-180 &&
           std::min(old.tangent,next.tangent)<=200 && std::max(old.tangent,next.tangent)>=0;
}

bool hitsClearance(Local old, Local next, int clearance, int extent, int margin)
{
    if (old.normal<clearance || next.normal>=clearance) return false;
    const int tangent=old.tangent+
        int(std::int64_t(next.tangent-old.tangent)*(clearance-old.normal)/(next.normal-old.normal));
    return tangent>=-margin && tangent<=extent+margin;
}

std::optional<double> crossingDistance(const game_lab::Sector& sector, const game_lab::Wall& wall,
                                       collision::Point start, collision::Point motion)
{
    const auto a=game_lab::worldVertex(sector,wall.vertexA);
    const auto b=game_lab::worldVertex(sector,wall.vertexB);
    const collision::Point edge{b.x-a.x,b.y-a.y};
    if (cross(edge,motion)<=0 || cross(edge,{start.x+motion.x-a.x,start.y+motion.y-a.y})<0)
        return {};
    return collision::rayDistance(collision::Segment{a,b},start,motion,1);
}

// Only follow connections intersected along the same proposal. A full-world
// sweep could stop movement at a door in an unrelated overlapping sector.
bool blockedBeyond(const game_lab::Level& level, const std::vector<doors::Door>& doors,
                   unsigned sectorIndex, collision::Point start, collision::Point motion,
                   double after)
{
    for (std::size_t hop=0;hop<level.sectors.size();++hop) {
        const auto& sector=level.sectors[sectorIndex];
        std::optional<std::size_t> first;
        double nearest=1;
        for (std::size_t i=0;i<sector.neighbors.size() && i<sector.walls.size();++i) {
            const auto& wall=sector.walls[i];
            if (sector.neighbors[i]>=level.sectors.size() ||
                wall.vertexA>=sector.vertices.size() || wall.vertexB>=sector.vertices.size()) continue;
            const auto hit=crossingDistance(sector,wall,start,motion);
            if (hit && *hit>after && *hit<=nearest) { first=i; nearest=*hit; }
        }
        if (!first) return false;
        const auto& wall=sector.walls[*first];
        if ((wall.type&0xf000)==0x1000 && wall.extra<level.connectionStates.size() &&
            !doors::dos::passageBranchEnabled(level.connectionStates[wall.extra]) &&
            std::any_of(doors.begin(),doors.end(),[&](const auto& door) {
                return door.stateIndex==wall.extra;
            })) return true;
        sectorIndex=sector.neighbors[*first];
        after=nearest;
    }
    return false;
}
}

collision::Point doors::moveEnemy(const game_lab::Level& level, const std::vector<Door>& doors,
                                  const collision::World& walls, unsigned sectorIndex,
                                  collision::Point before, collision::Point displacement,
                                  bool connectionMode)
{
    const auto swept=collision::move(walls,before,displacement);
    auto candidate=swept;
    if (doors.empty() || sectorIndex>=level.sectors.size()) return candidate;
    const auto& sector=level.sectors[sectorIndex];
    const int oldX=word(int(std::lround(before.x-sector.gx)));
    const int oldY=word(int(std::lround(before.y-sector.gy)));
    int x=word(int(std::lround(candidate.x-sector.gx)));
    int y=word(int(std::lround(candidate.y-sector.gy)));
    for (std::size_t i=0;i<sector.neighbors.size() && i<sector.walls.size();++i) {
        const auto& wall=sector.walls[i];
        if ((wall.type&0xf000)!=0x1000 || wall.extra>=level.connectionStates.size() ||
            wall.vertexA>=sector.vertices.size() || wall.vertexB>=sector.vertices.size()) continue;
        const auto state=level.connectionStates[wall.extra];
        if (dos::passageBranchEnabled(state)) continue;
        const auto a=sector.vertices[wall.vertexA], b=sector.vertices[wall.vertexB];
        const int dx=word(int(b.x)-a.x), dy=word(int(b.y)-a.y);
        if (!(dx || dy) || int(a.x)<x-300 || int(b.x)<x-300 || int(a.x)>x+300 || int(b.x)>x+300 ||
            int(a.y)<y-300 || int(b.y)<y-300 || int(a.y)>y+300 || int(b.y)>y+300) continue;
        if (std::none_of(doors.begin(),doors.end(),[&](const auto& door) {
                return door.stateIndex==wall.extra;
            })) continue;
        const int rotation=(900-wallAngle(dx,dy))%720;
        const auto old=rotate(word(oldX-a.x),word(oldY-a.y),rotation);
        const auto next=rotate(word(x-a.x),word(y-a.y),rotation);
        if (connectionMode && (!nearDoor(old,next) || next.normal<0)) continue;
        // 0x108b1, 0x10a4a: normal clearance is 80 in generic actor mode
        // and 180 in the connection branch. Only player mode centers tangent.
        const int clearance=connectionMode?180:80;
        const bool activeConnection=connectionMode && dos::connected(state);
        if (!activeConnection && !hitsClearance(old,next,clearance,rotate(dx,dy,rotation).tangent,
                                              dx==0 || dy==0 ? 80 : 30)) continue;
        const auto corrected=rotate(clearance+(activeConnection?0:2),next.tangent,(720-rotation)%720);
        x=word(corrected.normal+a.x);
        y=word(corrected.tangent+a.y);
        candidate={double(sector.gx)+x,double(sector.gy)+y};
    }
    if (candidate.x!=swept.x || candidate.y!=swept.y) {
        // The correction can send an actor away from the door when it starts
        // inside the clearance. Keep static collision on that new path.
        return collision::move(walls,before,{candidate.x-before.x,candidate.y-before.y});
    }
    return candidate;
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
    projectileLeaves_.clear();
    for (const auto& door:doors_) {
        if (door.stateIndex>=level_.connectionStates.size()) continue;
        if (!doors::dos::passageBranchEnabled(level_.connectionStates[door.stateIndex])) {
            const auto& f=door.faces[0];
            movementWorld_.push_back({{f.a.x,f.a.y},{f.b.x,f.b.y}});
        }
        for (const auto& closedFace:door.faces) {
            if (const auto face=doors::visibleFace(level_,door,closedFace))
                projectileLeaves_.push_back({face->a,face->b});
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
    const int heading=(int(std::lround(std::remainder(proposed.angle,2*vortex::math::pi)*360/vortex::math::pi))+720)%720;
    bool locked=false;
    const collision::Point start{before.x,before.y};
    const auto rejectCrossing=[&] {
        proposed.x=before.x;
        proposed.y=before.y;
        return locked;
    };
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
        const collision::Point motion{proposed.x-before.x,proposed.y-before.y};
        const auto hit=crossingDistance(sector,wall,start,motion);
        const auto hitsBlockedNeighbor=[&] {
            return hit && blockedBeyond(level_,doors_,sector.neighbors[i],start,motion,*hit);
        };
        if (found==doors_.end() || (wall.type&0xf000)!=0x1000 || wall.extra>=level_.connectionStates.size()) {
            // Ordinary open connections keep existing movement, but own the
            // sector transition. No guessed automatic requests for special walls.
            const auto extent=rotate(word(int(b.x)-a.x),word(int(b.y)-a.y),rotation).tangent;
            if (hitsBlockedNeighbor()) return rejectCrossing();
            if (old.normal>=0 && next.normal<0 && next.tangent>=0 && next.tangent<=extent) {
                proposed.sector=sector.neighbors[i]; break;
            }
            continue;
        }
        // 0x107db..0x10823: wall endpoints within proposed-position bounds.
        const bool near=int(a.x)>=x-300 && int(b.x)>=x-300 && int(a.x)<=x+300 && int(b.x)<=x+300 &&
                        int(a.y)>=y-300 && int(b.y)>=y-300 && int(a.y)<=y+300 && int(b.y)<=y+300 &&
                        nearDoor(old,next);
        auto& state=level_.connectionStates[wall.extra];
        const int relative=(heading-angle+1080)%720-360;
        const bool facing=relative>=120 && relative<240;
        if (near && !(wall.type&0x0800) && facing) {
            if (state&doors::dos::locked) locked=true;
            else state|=doors::dos::opening;
        }
        // The aggregate port proposal can skip the DOS clearance line or the
        // endpoint bounds. Reject a crossing of the actual non-passable face
        // before committing coordinates with the source sector still selected.
        const bool passable=doors::dos::passageBranchEnabled(state);
        if (hit && (!passable || hitsBlockedNeighbor())) return rejectCrossing();
        if (!near) continue;
        bool corrected=false, crossed=false, preserveFraction=false;
        if (passable) {
            crossed=next.normal<0;
            if (next.normal>=-180) state=(state&doors::dos::locked)|0x0140;
            if (next.tangent<80) next.tangent=82;
            else if (next.tangent>120) next.tangent=118;
            else preserveFraction=true;
            corrected=true;
        } else if (next.normal>=0) {
            if (state&0x01ff) {
                next.tangent+=next.tangent<100 ? std::min(10,100-next.tangent) : -std::min(10,next.tangent-100);
                next.normal=180; corrected=true;
            } else {
                // Closed/unrequested wall branch: intersect its clearance line.
                const int margin=(a.x==b.x || a.y==b.y)?80:30;
                const int extent=rotate(word(int(b.x)-a.x),word(int(b.y)-a.y),rotation).tangent;
                if (hitsClearance(old,next,180,extent,margin)) { next.normal=182; corrected=true; }
            }
        }
        if (corrected) {
            const auto result=rotate(next.normal,next.tangent,(720-rotation)%720);
            const int correctedX=word(result.normal+a.x), correctedY=word(result.tangent+a.y);
            if (preserveFraction) {
                // Keep the DOS integer correction and the port's sub-unit motion
                // through an open passage that has no tangential clamp.
                proposed.x+=word(correctedX-x);
                proposed.y+=word(correctedY-y);
            } else {
                proposed.x=double(sector.gx)+correctedX;
                proposed.y=double(sector.gy)+correctedY;
            }
            x=correctedX; y=correctedY;
        }
        if (crossed) { proposed.sector=sector.neighbors[i]; break; }
    }
    return locked;
}

const collision::World& DoorControl::movementWorld() const
{
    return movementWorld_;
}

const collision::World& DoorControl::staticWorld() const
{
    return staticWorld_;
}

const collision::World& DoorControl::projectileLeaves() const
{
    return projectileLeaves_;
}

std::uint16_t DoorControl::gameplayTicks(Time now)
{
    counter_.advance(now);
    return counter_.value();
}
