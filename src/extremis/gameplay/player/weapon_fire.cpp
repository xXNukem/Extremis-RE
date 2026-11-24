#include "extremis/world/pit_clock.h"
#include "extremis/diagnostics/shot_trace.h"
#include "extremis/gameplay/player/weapon_fire.h"
#include "extremis/resources/spr_format.h"
#include "extremis/gameplay/enemies/enemy_system.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <limits>

namespace player {
namespace trace = extremis::shot_trace;
namespace {
// Port reference cadence. The original loop does not establish a fixed wall-clock period.
constexpr double shotStep=1.0/60;
constexpr double maxShotElapsed=8*shotStep;
constexpr double timeEpsilon=1e-12;
constexpr double halfSecond=80.0*extremis::pit::divisor/extremis::pit::inputHz; // EXE PIT divisor and 80 IRQ window.
}

void WeaponFire::loadGraphics(const std::filesystem::path& directory, unsigned paletteBank)
{
    const auto palette=wall_textures::loadPalette(directory/("PAL"+std::to_string(paletteBank)+".PAL"));
    sprites::Bank bank;
    for (unsigned weapon=0;weapon<6;++weapon) {
        const auto source=spr_format::load(directory/("BALLE"+std::to_string(weapon)+".SPR"));
        if (source.size()!=(weapon==5?6u:4u)) throw std::runtime_error("Unexpected original BALLE bank");
        for (unsigned i=0;i<source.size();++i) {
            const auto& s=source[i]; const auto& f=weaponDefinitions[weapon].frames[i];
            sprites::Image image{{f.width,f.height,{}},f.anchorX,f.anchorY,{}};
            // DOS scales using descriptor dimensions, which differ from BALLE3[0].
            for (int y=0;y<f.height;++y) for (int x=0;x<f.width;++x) {
                const auto index=s.indices[(y*s.height/f.height)*s.width+x*s.width/f.width];
                image.indices.push_back(index); image.texture.pixels.push_back(palette[index]);
            }
            bank.images.push_back(std::move(image));
        }
    }
    scene_.bank=std::move(bank); scene_.objects.reserve(20);
}

std::size_t WeaponFire::activeProjectiles() const
{
    return std::count_if(projectiles_.begin(),projectiles_.end(),[](const auto& p){return p.state!=0;});
}
void WeaponFire::measureRate(double seconds)
{
    if (rateTime_+seconds>=halfSecond) {
        // [1424] counts frames; IRQ samples twice the count and resets it to ONE.
        rate_=std::max(9u,frames_*2);
        if (rateTime_+seconds>=halfSecond*2) rate_=9; // Empty subsequent IRQ windows.
        rateTime_=std::fmod(rateTime_+seconds,halfSecond); frames_=1;
    } else rateTime_+=seconds;
}
FireResult WeaponFire::attempt(const Camera& camera, Inventory& inventory)
{
    if (inventory.ammunition()==0) return FireResult::Empty;
    const auto free=std::find_if(projectiles_.begin(),projectiles_.end(),[](const auto& p){return p.state==0;});
    if (free==projectiles_.end()) return FireResult::Full;
    if (!inventory.consumeShot()) return FireResult::Empty;
    const auto& d=weaponDefinitions[weapon_];
    phase_=(phase_+1)&3;
    // Port aiming convention: projectile line is the optical axis, not DOS's
    // camera-30 plane / alternating parallel barrel offsets. Keep visual recoil.
    *free={1,weapon_,0,camera.x,camera.y,camera.angle};
    free->launchHeight=camera.viewHeight();
    trace::event("create", "slot=%td weapon=%u x=%.6f y=%.6f angle=%.6f",
                 free-projectiles_.begin(), weapon_, free->x, free->y, free->angle);
    // Project the center of the ORIGINAL ARME[2] firing layer back into world
    // coordinates at the first native projectile step. No guessed world offset.
    constexpr std::array<std::array<int,2>,6> flashSizes{{{14,11},{13,12},{13,13},{14,21},{28,18},{15,25}}};
    const unsigned barrel=phase_==3?1:phase_;
    const auto offset=d.muzzle[d.alternating?barrel:0];
    const double muzzleX=d.gunX+offset[0]+flashSizes[weapon_][0]*0.5;
    const double muzzleY=d.gunY+5+offset[1]+flashSizes[weapon_][1]*0.5;
    const double focal=160/std::tan(Camera::horizontalFov*.5);
    free->visualSide=(muzzleX-160)*d.speed/focal;
    free->visualDrop=(muzzleY-75)*d.speed/focal;
    cooldown_=d.wait*rate_/18+1;
    flashTime_=shotStep;
    return FireResult::Fired;
}
void WeaponFire::advanceProjectiles(const Camera& camera, const collision::World& walls,
                                    enemies::System* enemies, const collision::World& doorLeaves)
{
    const auto surfaceDistance=[&](collision::Point origin, collision::Point direction, double range, bool* hitsDoor = nullptr) {
        const auto wall=collision::rayDistance(walls,origin,direction,range);
        const auto door=collision::rayDistance(doorLeaves,origin,direction,wall.value_or(range),
                                               collision::Point{camera.x,camera.y});
        if (hitsDoor) *hitsDoor=door.has_value();
        return door ? door : wall;
    };
    scene_.objects.clear();
    for (auto& p:projectiles_) {
        if (!p.state) continue;
        const auto slot=&p-projectiles_.data();
        auto shotView=camera;
        shotView.postureHeight=p.launchHeight;
        shotView.bobPhase=0; // launchHeight already includes the camera offset at firing.
        std::optional<::enemies::ShotHit> enemyImpact;
        if (p.state==1) {
            const double distance=weaponDefinitions[p.weapon].speed;
            const collision::Point direction{std::cos(p.angle),std::sin(p.angle)};
            bool hitsDoor=false;
            const auto wall=surfaceDistance({p.x,p.y},direction,distance,&hitsDoor);
            const auto enemy=enemies ? enemies->trace(shotView,{p.x,p.y},direction,wall.value_or(std::nextafter(distance,std::numeric_limits<double>::infinity()))) : std::nullopt;
            const auto hit=enemy ? std::optional<double>{enemy->distance} : wall;
            const double travel=hit?std::max(0.0,*hit-(enemy?0:collision::Settings{}.skin)):distance;
            enemyImpact=enemy;
            p.x+=direction.x*travel; p.y+=direction.y*travel; p.traveled+=travel;
            if (hit) {
                p.state=2; p.phase=1; p.wallImpact=!enemy;
                trace::event("impact", "slot=%td target=%s x=%.6f y=%.6f", slot,
                             enemy?"enemy":hitsDoor?"door":"wall", p.x, p.y);
            }
        }
        const double dx=p.x-camera.x, dy=p.y-camera.y;
        const double forward=dx*std::cos(camera.angle)+dy*std::sin(camera.angle);
        const double side=dx*std::sin(camera.angle)-dy*std::cos(camera.angle);
        const double depth=forward+50;
        trace::event("step", "slot=%td state=%u phase=%u x=%.6f y=%.6f traveled=%.6f forward=%.6f",
                     slot,p.state,p.phase,p.x,p.y,p.traveled,forward);
        // EXE 0x17d1d..0x17d5c frees behind/outside-view slots.
        // Guard the signed-word coordinate domain; exact DOS wrap/quantization is not emulated.
        if (forward<=0 || std::abs(dx)>=32768 || std::abs(dy)>=32768) {
            trace::event("remove", "slot=%td reason=forward_or_range", slot);
            p.state=0; continue;
        }
        const double screenX=160+side*200/depth;
        if (screenX<0 || screenX>=320) {
            trace::event("remove", "slot=%td reason=horizontal_cull", slot);
            p.state=0; continue;
        }
        if (enemyImpact) {
            const auto index=enemyImpact->enemy;
            const unsigned before=trace::mode ? enemies->records()[index].health : 0;
            enemies->hit(index,p.weapon,p.angle);
            if (trace::mode)
                trace::event("damage", "slot=%td enemy=%zu before=%u after=%u",
                             slot,index,before,unsigned(enemies->records()[index].health));
        }
        const auto& d=weaponDefinitions[p.weapon];
        const unsigned frame=d.stages[p.phase];
        double blend=0;
        if (p.state==1) {
            auto remaining=surfaceDistance({p.x,p.y},
                {std::cos(p.angle),std::sin(p.angle)},32767);
            if (enemies) {
                const auto target=enemies->trace(shotView,{p.x,p.y},{std::cos(p.angle),std::sin(p.angle)},remaining.value_or(32767));
                if (target) remaining=target->distance;
            }
            const double extent=remaining.value_or(0)+std::max(0.0,p.traveled-d.speed);
            blend=remaining ? (extent>0 ? *remaining/extent : 0) : 1;
        }
        const double visualX=p.x+std::sin(p.angle)*p.visualSide*blend;
        const double visualY=p.y-std::cos(p.angle)*p.visualSide*blend;
        // Collision and impacts stay on the aiming line. Only flight sprites
        // converge from the muzzle to the nearest real surface, in world space.
        scene_.objects.push_back({visualX,visualY,p.launchHeight-p.visualDrop*blend,
                                  double(d.frames[frame].worldHeight),p.weapon*4+frame,false,false,
                                  p.wallImpact,p.state==2 && !p.wallImpact});
        // EXE 0x180e9..0x180fe: impact phases 1..5, then FREE, no invented TTL.
        trace::event("sprite", "slot=%td state=%u phase=%u image=%u x=%.6f y=%.6f height=%.6f",
                     slot,p.state,p.phase,p.weapon*4+frame,visualX,visualY,p.launchHeight-p.visualDrop*blend);
        if (p.state==2 && ++p.phase==6) {
            p.state=0;
            trace::event("remove", "slot=%td reason=last_phase_queued", slot);
        }
    }
}
FireResult WeaponFire::update(const Camera& camera, const collision::World& walls, Inventory& inventory,
                             FireTrigger trigger, bool active, double seconds, enemies::System* enemies,
                             const collision::World& doorLeaves)
{
    if (!active || !std::isfinite(seconds) || seconds<0 ||
        !std::isfinite(camera.x) || !std::isfinite(camera.y) || !std::isfinite(camera.angle)) {
        blocked_=true; pending_=wasActive_=false; return FireResult::None;
    }
    if (inventory.weapon()>=weaponDefinitions.size()) return FireResult::None;
    if (weapon_!=inventory.weapon()) {
        weapon_=inventory.weapon();
        cooldown_=phase_=0;
        flashTime_=0;
    }
    const double elapsed=wasActive_?seconds:0;
    measureRate(elapsed);
    wasActive_=true;

    const double shotElapsed=std::min(elapsed,maxShotElapsed);
    flashTime_=std::max(0.0,flashTime_-shotElapsed);
    // Keep the last queued impact image for its full step, even after its slot is free.
    bool shotInProgress=!scene_.objects.empty() || activeProjectiles()!=0;
    if (shotInProgress) {
        projectileTime_+=shotElapsed;
        while (projectileTime_+timeEpsilon>=shotStep) {
            projectileTime_=std::max(0.0,projectileTime_-shotStep);
            advanceProjectiles(camera,walls,enemies,doorLeaves);
            shotInProgress=!scene_.objects.empty() || activeProjectiles()!=0;
            if (!shotInProgress) {
                projectileTime_=0;
                break;
            }
        }
    }

    // Trigger admission and the DOS-inspired cooldown are independent of shot steps.
    if (!trigger.held && !trigger.pressed) { blocked_=false; pending_=false; }
    if (trigger.pressed) { blocked_=false; pending_=true; }
    FireResult result=FireResult::None;
    if (cooldown_) --cooldown_;
    else if (!blocked_ && (weaponDefinitions[weapon_].automatic?(trigger.held || trigger.pressed):pending_)) {
        result=attempt(camera,inventory);
        if (!weaponDefinitions[weapon_].automatic && (result==FireResult::Fired || result==FireResult::Empty)) {
            blocked_=true; pending_=false;
        }
    }
    // Launch the first shot of an idle sequence at once. Shots admitted into a
    // running sequence join its next tick, never a catch-up tick in the past.
    if (result==FireResult::Fired && !shotInProgress)
        advanceProjectiles(camera,walls,enemies,doorLeaves);
    ++frames_;
    return result;
}
}

namespace player {
bool WeaponFire::firing() const
{
    return flashTime_>timeEpsilon;
}

unsigned WeaponFire::barrelPhase() const
{
    return phase_;
}

unsigned WeaponFire::estimatedRate() const
{
    return rate_;
}

unsigned WeaponFire::cooldown() const
{
    return cooldown_;
}

const sprites::Scene& WeaponFire::scene() const
{
    return scene_;
}
} // namespace player
