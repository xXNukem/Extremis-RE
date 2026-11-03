#include "weapon_fire.h"
#include "sprite_renderer.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
using namespace player;
void check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
void equip(Inventory& inventory, unsigned weapon, unsigned ammo)
{
    auto contents=std::uint16_t((weapon<<12)|ammo);
    check(inventory.exchangeWeapon(contents)==Transfer::Exchanged,"Test cabinet exchange failed");
}
void triggersAndAmmo()
{
    for (unsigned id=0;id<6;++id) {
        Inventory inventory; equip(inventory,id,100);
        WeaponFire fire; Camera camera{};
        check(fire.update(camera,{},inventory,{true,true},true,0)==FireResult::Fired,"Fresh click did not fire");
        check(fire.firing() && inventory.ammunition()==99,"Accepted shot lost pose or charged wrong ammo");
        unsigned count=1;
        for (int i=0;i<60;++i) {
            if (fire.update(camera,{},inventory,{true,false},true,1./60)==FireResult::Fired) ++count;
        }
        check(weaponDefinitions[id].automatic?count>1:count==1,"Semi/automatic held-button rule differs");
        check(inventory.ammunition()==100-int(count),"Held fire duplicated or skipped ammo consumption");
        fire.update({0,0,3.141592653589793},{},inventory,{},true,0); // Retire prior shots, not a full-pool re-press.
        for (int i=0;i<100;++i) fire.update(camera,{},inventory,{},true,0);
        check(fire.update(camera,{},inventory,{true,true},true,0)==FireResult::Fired,"Release/repress failed");
    }
    Inventory empty; equip(empty,0,0); WeaponFire fire;
    check(fire.update({}, {},empty,{true,true},true,0)==FireResult::Empty &&
          empty.ammunition()==0 && fire.activeProjectiles()==0 && !fire.firing(),"Empty weapon emitted/charged/animated a shot");
}
void clockAndCooldown()
{
    for (int fps:{30,60,144}) {
        Inventory inventory; equip(inventory,1,100); WeaponFire fire;
        for (int i=0;i<fps+1;++i) fire.update({}, {},inventory,{},true,1./fps);
        check(fire.estimatedRate()>=unsigned(fps-2) && fire.estimatedRate()<=unsigned(fps+4),
              "Half-second frame estimator is not tracking game iterations");
        check(fire.update({}, {},inventory,{true,true},true,0)==FireResult::Fired,"Clock test shot failed");
        const auto wait=weaponDefinitions[1].wait*fire.estimatedRate()/18+1;
        check(fire.cooldown()==wait,"EXE floor(k*FPS/18)+1 formula differs");
        for (unsigned i=0;i<wait;++i)
            check(fire.update({}, {},inventory,{true,false},true,0)==FireResult::None,"Cooldown expired one iteration early");
        check(fire.update({}, {},inventory,{true,false},true,0)==FireResult::Fired,"Cooldown did not expire at original iteration");
        const auto ammo=inventory.ammunition(); const auto rate=fire.estimatedRate();
        fire.update({}, {},inventory,{true,true},false,100);
        for (int i=0;i<100;++i) fire.update({}, {},inventory,{true,false},false,100);
        fire.update({}, {},inventory,{true,false},true,100);
        check(inventory.ammunition()==ammo && fire.estimatedRate()==rate,"Inactive interval leaked a shot or clock catch-up");
        fire.update({}, {},inventory,{},true,0);
        const auto remaining=fire.cooldown();
        for (unsigned i=0;i<remaining+1;++i) fire.update({}, {},inventory,{},true,0);
        check(fire.update({}, {},inventory,{true,true},true,0)==FireResult::Fired,"Post-modal release and click did not rearm");
    }
}
void projectileLifecycle()
{
    Inventory inventory; equip(inventory,5,100); WeaponFire fire;
    while (fire.activeProjectiles()<20) fire.update({}, {},inventory,{true,true},true,0);
    check(inventory.ammunition()==80,"Twenty slots did not cost twenty units");
    FireResult result=FireResult::None;
    for(int i=0;i<4 && result!=FireResult::Full;++i)
        result=fire.update({}, {},inventory,{true,false},true,0);
    check(result==FireResult::Full && inventory.ammunition()==80,"Full pool consumed ammo or admitted a 21st projectile");
    fire.update({0,0,3.141592653589793}, {},inventory,{},true,0);
    check(fire.activeProjectiles()==0,"Turning away did not release offscreen/behind projectiles");
    check(fire.update({}, {},inventory,{true,true},true,0)==FireResult::Fired,"Released slot could not be reused");

    // Original impact sequence 1..5 maps to 1,2,1,2,3 (laser:1,2,3,4,5).
    for(unsigned id=0;id<6;++id) {
        WeaponFire impact; Inventory ammo; equip(ammo,id,10);
        const collision::World wall{{{30,-1000},{30,1000}}};
        check(impact.update({},wall,ammo,{true,true},true,0)==FireResult::Fired,"Near-wall shot rejected");
        for(unsigned phase=1;phase<=5;++phase) {
            check(impact.scene().objects.size()==1 &&
                  impact.scene().objects[0].image==id*4+weaponDefinitions[id].stages[phase],"Wrong original BALLE impact frame");
            if(phase<5) impact.update({},wall,ammo,{},true,0);
        }
        check(impact.activeProjectiles()==0,"Impact left an immortal occupied slot");
        impact.update({},wall,ammo,{},true,0);
        check(impact.scene().objects.empty(),"Finished effect remained in sprite scene");
    }
    WeaponFire outbound; Inventory ammo;
    outbound.update({}, {},ammo,{true,true},true,0);
    for(int i=0;i<500;++i) outbound.update({}, {},ammo,{},true,0);
    check(outbound.activeProjectiles()==0,"Unbounded map flight never released its word-range slot");
}
void exchangeAndBlocking()
{
    for(int context=0;context<4;++context) { // Map, death, no focus, modal all gate active=false.
        Inventory inventory; WeaponFire fire;
        fire.update({}, {},inventory,{true,true},false,1);
        fire.update({}, {},inventory,{true,false},true,1);
        check(inventory.ammunition()==500 && !fire.firing(),"Blocked/context-restored held click fired");
        fire.update({}, {},inventory,{},true,0);
        check(fire.update({}, {},inventory,{true,true},true,0)==FireResult::Fired,"New click after context did not fire");
    }
    Inventory inventory; WeaponFire fire;
    fire.update({}, {},inventory,{true,true},true,0);
    auto cabinet=std::uint16_t(0x11f4);
    inventory.exchangeWeapon(cabinet);
    check(cabinet==499 && inventory.weapon()==1 && inventory.ammunition()==500,"Shooting changed cabinet exchange semantics");
    fire.update({}, {},inventory,{},true,0);
    fire.update({}, {},inventory,{true,true},true,0);
    check(inventory.ammunition()==499,"Equipped weapon did not own shot ammunition");
}
void muzzleAndImpactProjection()
{
    const double pi=3.14159265358979323846;
    const sprites::Image image{{5,9,std::vector<wall_textures::Rgb>(45)},2,3,std::vector<std::uint8_t>(45,1)};
    for(unsigned id=0;id<6;++id) for(double angle:{0.,pi/2,pi,-pi/2,0.39}) {
        Inventory inventory;equip(inventory,id,20);WeaponFire fire;Camera camera{1000,1000,angle};
        fire.update(camera,{},inventory,{true,true},true,0);
        constexpr int flashWidth[]{14,13,13,14,28,15};
        constexpr int flashHeight[]{11,12,13,21,18,25};
        const auto& d=weaponDefinitions[id];
        const auto offset=d.muzzle[d.alternating?1:0];
        check(fire.scene().objects.size()==1,"Missing muzzle projectile");
        const auto& point=fire.scene().objects[0];
        const auto rect=sprites::project(point,image,camera,{320,150,160/std::tan(35*pi/180),Camera::eyeHeight});
        check(rect && std::abs(rect->left+rect->width/5*2-(d.gunX+offset[0]+flashWidth[id]*.5))<1e-8 &&
              std::abs(rect->top+rect->height/9*3-(d.gunY+5+offset[1]+flashHeight[id]*.5))<1e-8,
              "Projectile does not originate at original firing muzzle");
        WeaponFire impact;Inventory ammo;equip(ammo,id,10);
        const double dx=std::cos(angle),dy=std::sin(angle);
        const collision::Point center{camera.x+dx*200,camera.y+dy*200};
        const collision::World wall{{{center.x-dy*100,center.y+dx*100},{center.x+dy*100,center.y-dx*100}}};
        impact.update(camera,wall,ammo,{true,true},true,0);
        for(int step=0;step<5;++step) impact.update(camera,wall,ammo,{},true,0);
        check(!impact.scene().objects.empty(),"Impact projection test has no visible impact");
        for(const auto& point:impact.scene().objects) {
            const auto rect=sprites::project(point,image,camera,{320,150,200,Camera::eyeHeight});
            check(rect && std::abs(rect->top+rect->height/9*3-75)<1e-8 &&
                  std::abs(rect->left+rect->width/5*2-160)<1e-8,"Impact projection does not align with reticle");
        }
    }
}
void originals(const std::filesystem::path& directory)
{
    WeaponFire fire; fire.loadGraphics(directory,1);
    check(fire.scene().bank.images.size()==26,"Original BALLE banks not loaded completely");
    std::ifstream file(directory.parent_path()/"EXTREMIS.EXE",std::ios::binary);
    const std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(file),{}};
    const auto word=[&](unsigned offset){return int(std::int16_t(bytes.at(offset)|(unsigned(bytes.at(offset+1))<<8)));};
    for(unsigned id=0;id<6;++id) {
        const auto& d=weaponDefinitions[id];const auto base=0x56d7+112*id;
        check(d.wait==unsigned(word(base+28)) && d.automatic==(word(base+26)!=0) &&
              d.speed==word(base+32) && d.alternating==(word(base+12)!=0) && d.aimX==word(base+4) && d.aimY==word(base+6) &&
              d.gunX==word(base+8) && d.gunY==word(base+10),"Weapon header differs from EXE descriptor");
        for(unsigned frame=0;frame<6;++frame) {
            const auto& f=d.frames[frame];
            check(f.width==word(base+40+frame*10) && f.height==word(base+42+frame*10) &&
                  f.anchorX==word(base+44+frame*10) && f.anchorY==word(base+46+frame*10) &&
                  f.worldHeight==word(base+48+frame*10),"Projectile geometry differs from EXE descriptor");
        }
        for(unsigned stage=0;stage<6;++stage)
            check(d.stages[stage]==unsigned(word(base+100+stage*2)),"Projectile stage map differs from EXE");
        for(unsigned phase=0;phase<3;++phase)
            check(d.muzzle[phase][0]==word(base+14+phase*4) && d.muzzle[phase][1]==word(base+16+phase*4),
                  "Muzzle offsets differ from EXE");
    }
}
}
int main(int argc,char** argv)
{
    try {
        triggersAndAmmo();clockAndCooldown();projectileLifecycle();exchangeAndBlocking();muzzleAndImpactProjection();
        if(argc==2) originals(argv[1]);
        std::cout<<"Equipped weapon fire, original clock, 20-slot lifecycle, gating and exchange passed.\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
