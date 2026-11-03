#include "enemy_effects.h"
#include <algorithm>
#include <cmath>

namespace enemies {
namespace {
// EXE file 0x5ad5, bank1 descriptor: seven triples followed by four fragments.
constexpr int bursts[7][3]{{3,50,200},{3,-20,170},{2,40,140},{1,-30,110},
                            {0,30,80},{0,-40,50},{2,20,20}};
constexpr int pieces[4][3]{{16,-60,210},{17,60,210},{18,-60,40},{19,60,40}};
constexpr unsigned burstFrames[]{15,14,13,12,13,14,15}; // File0x100ef, BYTE table.
}
void DeathEffects::reset()
{
    phase_.reset(); slime_.reset(); objects_.clear(); fragments_={};
}
void DeathEffects::splashNear(collision::Point point, const Camera& camera)
{
    // EXE 0xfdd4..0xfdfd /0x10013..0x1003c; no line-of-sight condition.
    if (std::max(std::abs(point.x-camera.x),std::abs(point.y-camera.y))<400) slime_=0;
}
void DeathEffects::begin(collision::Point point, double elevation, const Camera& camera)
{
    origin_=point; elevation_=elevation;
    heading_=std::atan2(camera.y-point.y,camera.x-point.x);
    phase_=0; fragments_={}; objects_.clear();
    for (unsigned i=0;i<4;++i) {
        fragments_[i]={{origin_.x-std::sin(heading_)*pieces[i][1],
                        origin_.y+std::cos(heading_)*pieces[i][1]},
                       elevation_+pieces[i][2],pieces[i][0]};
    }
    splashNear(point,camera);
}
void DeathEffects::step(const Camera& camera, const collision::World& walls)
{
    if (slime_ && ++*slime_>=30) slime_.reset();
    if (!phase_) return;
    if (*phase_<9) ++*phase_;
    // FD54 initializes on phase0; FEE1 runs during EVERY subsequent phase,
    // interleaved with the 10/11 body and seven bursts, not after phase9.
    for (auto& part:fragments_) {
        if (part.image<0) continue;
        if (part.image>=16) {
            const auto next=collision::move(walls,part.position,{100*std::cos(heading_),100*std::sin(heading_)});
            // EXE 0xff9d..0x10048: changed position starts the 12..15 burst.
            if (next.x!=part.position.x || next.y!=part.position.y) {
                part.position=next; part.image=12; splashNear(next,camera);
            }
        } else if (++part.image==16) part.image=-1;
    }
    // Equivalent retirement condition, including fragments that ended on earlier ticks.
    if (*phase_>=9 && std::all_of(fragments_.begin(),fragments_.end(),[](const auto& p){return p.image<0;})) phase_.reset();
}
void DeathEffects::compose(const Camera& camera, const sprites::Bank& bank)
{
    objects_.clear();
    if (!phase_ || bank.images.size()<20) return;
    const auto add=[&](collision::Point point,double z,unsigned image) {
        objects_.push_back({point.x,point.y,z,270.0*bank.images[image].texture.height/
            bank.images[0].texture.height,image,0,0});
    };
    if (*phase_<2) add(origin_,elevation_,10+*phase_);
    else if (*phase_<9) {
        for (const auto& burst:bursts) {
            const unsigned stage=*phase_-2+unsigned(burst[0]);
            if (stage<7) add({origin_.x+std::sin(camera.angle)*burst[1],
                             origin_.y-std::cos(camera.angle)*burst[1]},
                            elevation_+burst[2],burstFrames[stage]);
        }
    }
    if (*phase_>0) for (const auto& part:fragments_)
        if (part.image>=0) add(part.position,part.z,unsigned(part.image));
}
}
