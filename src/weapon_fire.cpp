#include "weapon_fire.h"
#include "spr_format.h"
#include "enemy_system.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <limits>

namespace player {
namespace {
constexpr double halfSecond=80.0*7457.0/1193182.0; // EXE PIT divisor and 80 IRQ window.
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
    firing_=true;
    return FireResult::Fired;
}
void WeaponFire::advanceProjectiles(const Camera& camera, const collision::World& walls, enemies::System* enemies)
{
    scene_.objects.clear();
    for (auto& p:projectiles_) {
        if (!p.state) continue;
        auto shotView=camera;
        shotView.postureHeight=p.launchHeight;
        std::optional<::enemies::ShotHit> enemyImpact;
        if (p.state==1) {
            const double distance=weaponDefinitions[p.weapon].speed;
            const collision::Point direction{std::cos(p.angle),std::sin(p.angle)};
            const auto wall=collision::rayDistance(walls,{p.x,p.y},direction,distance);
            const auto enemy=enemies ? enemies->trace(shotView,{p.x,p.y},direction,wall.value_or(std::nextafter(distance,std::numeric_limits<double>::infinity()))) : std::nullopt;
            const auto hit=enemy ? std::optional<double>{enemy->distance} : wall;
            const double travel=hit?std::max(0.0,*hit-(enemy?0:collision::Settings{}.skin)):distance;
            enemyImpact=enemy;
            p.x+=direction.x*travel; p.y+=direction.y*travel; p.traveled+=travel;
            if (hit) { p.state=2; p.phase=1; }
        }
        const double dx=p.x-camera.x, dy=p.y-camera.y;
        const double forward=dx*std::cos(camera.angle)+dy*std::sin(camera.angle);
        const double side=dx*std::sin(camera.angle)-dy*std::cos(camera.angle);
        const double depth=forward+50;
        // EXE 0x17d1d..0x17d5c frees behind/outside-view slots.
        // Guard the signed-word coordinate domain; exact DOS wrap/quantization is not emulated.
        if (forward<=0 || std::abs(dx)>=32768 || std::abs(dy)>=32768) {
            p.state=0; continue;
        }
        const double screenX=160+side*200/depth;
        if (screenX<0 || screenX>=320) {
            p.state=0; continue;
        }
        if (enemyImpact) enemies->hit(enemyImpact->enemy,p.weapon,p.angle);
        const auto& d=weaponDefinitions[p.weapon];
        const unsigned frame=d.stages[p.phase];
        double blend=0;
        if (p.state==1) {
            auto remaining=collision::rayDistance(walls,{p.x,p.y},
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
                                  double(d.frames[frame].worldHeight),p.weapon*4+frame,0,0});
        // EXE 0x180e9..0x180fe: impact phases 1..5, then FREE, no invented TTL.
        if (p.state==2 && ++p.phase==6) p.state=0;
    }
}
FireResult WeaponFire::update(const Camera& camera, const collision::World& walls, Inventory& inventory,
                             FireTrigger trigger, bool active, double seconds, enemies::System* enemies)
{
    firing_=false;
    if (!active || !std::isfinite(seconds) || seconds<0 ||
        !std::isfinite(camera.x) || !std::isfinite(camera.y) || !std::isfinite(camera.angle)) {
        blocked_=true; pending_=wasActive_=false; return FireResult::None;
    }
    if (inventory.weapon()>=weaponDefinitions.size()) return FireResult::None;
    if (weapon_!=inventory.weapon()) { weapon_=inventory.weapon(); cooldown_=phase_=0; }
    measureRate(wasActive_?seconds:0); wasActive_=true;
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
    advanceProjectiles(camera,walls,enemies);
    ++frames_;
    return result;
}
}
