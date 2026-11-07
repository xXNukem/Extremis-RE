#include "enemy_system.h"
#include "enemy_profiles.h"
#include "sprite_renderer.h"
#include "weapon_fire.h"
#include "enemy_navigation.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {
void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void write(const std::filesystem::path& path, const std::vector<unsigned char>& bytes)
{
    std::ofstream file(path,std::ios::binary);
    file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
    check(bool(file),"Cannot write fixture");
}
void fixture(const std::filesystem::path& directory, unsigned bank=1)
{
    std::filesystem::create_directories(directory);
    const unsigned count=enemies::original::profiles.at(bank-1).frames;
    const auto name="MONS"+std::to_string(bank);
    std::vector<unsigned char> bytes(4+count*4,0); bytes[2]=count;
    for (unsigned i=0;i<count;++i) {
        unsigned offset=bytes.size()-4;
        for (unsigned j=0;j<4;++j) bytes[4+i*4+j]=(offset>>(8*j))&255;
        bytes.insert(bytes.end(),{2,0,2,0,1,0,2,0,0,1,1,1});
    }
    write(directory/(name+".SPR"),bytes);
    write(directory/"PAL1.PAL",std::vector<unsigned char>(768,63));
    write(directory/(name+".SPT"),std::vector<unsigned char>(count*8,0));
}
void placement(const std::filesystem::path& directory)
{
    fixture(directory);
    game_lab::Level level{}; level.monsBank=level.palBank=1;
    level.sectors.resize(1); level.sectors[0].gx=1000; level.sectors[0].gy=2000;
    level.enemies={{4,2,6,0,0,300,100,0xffff,0,0,0x8000,0,300,100}};
    enemies::System system; system.load(level,directory);
    check(system.records().size()==1,"Missing enemy");
    const auto& object=system.scene().objects.at(0);
    check(object.x==1300 && object.y==2100 && object.height==270 && object.z==0,
          "Enemy coordinate/height conversion failed");
    check(object.image==0 && object.type==0,"Incorrect initial frame/flags");
    const Camera camera{1000,2100,0};
    const auto rect=sprites::project(object,system.scene().bank.images[0],camera,{320,150,200,128});
    check(rect && std::abs(rect->height-180)<1e-9,"Enemy scale incorrect");
    std::vector<std::uint32_t> pixels(320*150,0);
    std::vector<double> depth(pixels.size(),100); // A closed surface in front of enemy.
    sprites::render(pixels,depth,system.scene(),camera,{320,150,200,128});
    for (auto pixel:pixels) check(pixel==0,"Enemy drawn through wall/door depth");
    level.enemies[0].sprite=0xa000;
    system.load(level,directory); check(system.scene().objects[0].type==0x0200,"Mirror flag lost");
    level.enemies[0].health=0xffff;
    system.load(level,directory); check(system.scene().objects.empty(),"Removed enemy reappeared");
    level.enemies[0].health=4;
    system.load(level,directory); check(system.scene().objects.size()==1,"Level reset failed");
}
game_lab::Level arena()
{
    game_lab::Level level{}; level.monsBank=level.palBank=1;
    level.sectors.resize(1);
    auto& sector=level.sectors[0];
    sector.vertices={{0,0},{1000,0},{1000,1000},{0,1000}};
    sector.polygons={{{0,1,2,3},0,{}}};
    sector.walls={{0,1,1,0},{1,2,1,0},{2,3,1,0},{3,0,1,0}};
    level.enemies={{4,4,6,0,0,500,500,0xffff,360,0,0x8000,0,500,500}};
    return level;
}
void otherBanks(const std::filesystem::path& directory)
{
    fixture(directory);
    // One image proves unknown banks never index MONS1's action sequences.
    std::vector<unsigned char> bytes{0,0,1,0,4,0,0,0,2,0,2,0,1,0,2,0,0,1,1,1};
    write(directory/"MONS4.SPR", bytes);
    write(directory/"MONS4.SPT", std::vector<unsigned char>(8,0));
    auto level=arena(); level.monsBank=4;
    enemies::System system;
    const auto rejects=[&] {
        try { system.load(level,directory); } catch (const std::runtime_error&) { return; }
        throw std::runtime_error("Invalid MONS accepted");
    };
    rejects(); // A known bank cannot supply fewer frames than its native sequences.
    fixture(directory,4);
    level.enemies[0].sprite=0x8010; rejects();
    level.enemies[0].sprite=0x8000;
    write(directory/"MONS4.SPT", std::vector<unsigned char>(16,0)); rejects();
    write(directory/"MONS4.SPT", std::vector<unsigned char>(7,0)); rejects();
}
void allBanks(const std::filesystem::path& directory, bool synthetic)
{
    constexpr unsigned heights[]{270,170,190,160,180,200,270,200,270};
    constexpr unsigned counts[]{20,18,16,16,16,16,20,18,10};
    for(unsigned bank=1;bank<=9;++bank) {
        if(synthetic) fixture(directory,bank);
        auto level=arena(); level.monsBank=bank;
        const auto walls=collision::staticWorld(level);
        enemies::System system; system.load(level,directory);
        player::Vitals vitals; Camera view{450,500,0};
        check(system.scene().bank.images.size()==counts[bank-1] &&
              system.scene().objects[0].height==heights[bank-1],"Native bank profile differs");
        check(system.scene().objects[0].z==((bank==4 || bank==6)?256:0),"Native elevation missing");
        system.update(level,view,walls,vitals,0,true);
        for(unsigned i=0;i<40;++i) {
            system.update(level,view,walls,vitals,1./18,true);
            for(const auto& object:system.scene().objects)
                check(object.image<counts[bank-1],"Action frame out of bounds");
            system.trace(view,{view.x,view.y},{1,0},1000);
        }
        if(bank==5) check(vitals.health()==1000,"Partial MONS5 invented an attack");
        else check(vitals.health()<1000,"Enabled bank failed to attack");
        if(bank==5 || bank==6)
            check(system.records()[0].x==500 && system.records()[0].y==500,"Special bank used shared walking");
        system.hit(0,0,0);
        check(system.records()[0].health==3 && system.records()[0].state==4,"Bank did not receive hit");
        for(unsigned i=0;i<4;++i) system.update(level,view,walls,vitals,1./18,true);
        system.hit(0,4,0);
        for(unsigned i=0;i<8;++i) system.update(level,view,walls,vitals,1./18,true);
        check(system.records()[0].health==0xffff && system.scene().objects.empty(),"Bank did not retire");
        check(system.terminalDefeated()==(bank==9),"Boss ending flag differs");
        const auto sounds=system.takeSounds();
        check((std::find(sounds.begin(),sounds.end(),17)!=sounds.end())==(bank!=9),"Wrong death sound");
        if(bank==9) check(!system.effects().active(),"Boss used absent explosion frames");
        for(unsigned i=0;i<35;++i) {
            system.update(level,view,walls,vitals,1./18,true);
            for(const auto& object:system.effects().objects())
                check(object.image<counts[bank-1],"Bank used nonexistent fragment");
        }
        check(!system.effects().active(),"Bank effects did not retire");
        system.load(level,directory); check(!system.terminalDefeated(),"Ending flag survived reload");
        view.x=300;
        system.update(level,view,walls,vitals,0,true);
        system.update(level,view,walls,vitals,1./18,true);
        const unsigned expectedStep=(bank==5 || bank==6)?0:(bank==9?10:30);
        check(system.records()[0].x==500-expectedStep && system.records()[0].y==500,
              "Bank pursuit speed or stationary policy differs");
    }
    // MONS6 differs in both its attack sequence and its three-step hurt pose.
    auto level=arena(); level.monsBank=6;
    enemies::System system; system.load(level,directory);
    player::Vitals vitals; Camera view{450,500,0}; const auto walls=collision::staticWorld(level);
    system.update(level,view,walls,vitals,0,true);
    constexpr unsigned attackFrames[]{6,7,8,7,6,5};
    for(auto image:attackFrames) {
        system.update(level,view,walls,vitals,1./18,true);
        check((system.records()[0].sprite&255)==image,"MONS6 attack sequence differs");
    }
    system.hit(0,0,0);
    for(auto image:{9u,5u,9u}) {
        system.update(level,view,walls,vitals,1./18,true);
        check((system.records()[0].sprite&255)==image && system.records()[0].state==4,
              "MONS6 hurt sequence differs");
    }
}
void combat(const std::filesystem::path& directory, unsigned bank=1)
{
    fixture(directory,bank);
    auto level=arena(); level.monsBank=bank; auto walls=collision::staticWorld(level);
    enemies::System system; system.load(level,directory);
    player::Vitals vitals; Camera camera{300,500,0};
    auto hit=system.trace(camera,{300,500},{1,0},1000);
    check(hit && hit->enemy==0 && hit->distance==200,"Original mask/billboard trace failed");
    Camera attempted{450,500,0};
    system.constrainPlayer(level,{300,500,0},attempted,walls);
    check(attempted.x<400 && attempted.x>399.99,"Native enemy body did not block player");
    const auto edge=attempted;
    attempted.x+=50;
    system.constrainPlayer(level,edge,attempted,walls);
    check(attempted.x<400,"Enemy body leaked at its boundary on the following frame");

    system.hit(0,0,0);
    check(system.records()[0].health==3 && system.records()[0].state==4,"Weapon damage/hurt failed");
    system.hit(0,4,0);
    check(system.records()[0].health==0 && system.records()[0].state==5,"Lethal hit failed");
    system.update(level,camera,walls,vitals,0,true);
    for(unsigned i=0;i<8;++i) system.update(level,camera,walls,vitals,1./18,true);
    check(system.scene().objects.empty() && system.records()[0].health==0xffff,"Original death removal failed");
    check(system.effects().active() && !system.effects().objects().empty() &&
          !system.trace(camera,{camera.x,camera.y},{1,0},1000),"Death visuals changed combat targeting");

    attempted={450,500,0}; system.constrainPlayer(level,{300,500,0},attempted,walls);
    check(attempted.x==450,"Removed enemy still blocked player");

    const auto sounds=system.takeSounds();
    check(std::count(sounds.begin(),sounds.end(),17)==1,"Death sound duplicated or absent");
    for(unsigned i=0;i<20;++i) system.update(level,camera,walls,vitals,1./18,true);
    check(system.scene().objects.empty() && system.takeSounds().empty(),"Dead enemy respawned or repeated audio");
    system.load(level,directory); vitals.reset();
    system.update(level,camera,walls,vitals,0,true);
    const auto initialX=system.records()[0].x;
    for(unsigned i=0;i<4;++i) system.update(level,camera,walls,vitals,1./18,true);
    check(system.records()[0].x<initialX,"Enemy did not pursue player");
    check(vitals.health()==1000,"Damage occurred before attack phase");
    for(unsigned i=0;i<30 && vitals.health()==1000;++i) system.update(level,camera,walls,vitals,1./18,true);
    check(vitals.health()==998,"Attack phase2 did not start original damage reaction");
    camera={900,900,0};
    system.update(level,camera,walls,vitals,10,false);
    system.update(level,camera,walls,vitals,10,true);
    check(vitals.health()==998,"Modal/focus pause or resume applied damage");
    for(unsigned i=0;i<7;++i) system.update(level,camera,walls,vitals,1./18,true);
    check(vitals.health()==984,"Original reaction must apply 2 units for eight updates");
    // Same-sector but separated by a closed door/wall: no pursuit through it or attack.
    system.load(level,directory); vitals.reset(); camera={300,500,0};
    auto blocked=walls; blocked.push_back({{400,0},{400,1000}});
    system.update(level,camera,blocked,vitals,0,true);
    for(unsigned i=0;i<120;++i) system.update(level,camera,blocked,vitals,1./18,true);
    check(system.records()[0].x>=430 && vitals.health()==1000,"Closed surface failed to block movement/attacks");
    player::WeaponFire fire; player::Inventory inventory;
    fire.update(camera,blocked,inventory,{true,true},true,0,&system);
    for(unsigned i=0;i<10;++i) fire.update(camera,blocked,inventory,{},true,0,&system);
    check(system.records()[0].health==4 && inventory.ammunition()==499,"Shot crossed wall or charged twice");
    system.load(level,directory);
    player::WeaponFire clearFire; player::Inventory clearInventory;
    clearFire.update(camera,walls,clearInventory,{true,true},true,0,&system);
    for(unsigned i=0;i<10;++i) clearFire.update(camera,walls,clearInventory,{},true,0,&system);
    check(system.records()[0].health==3 && clearInventory.ammunition()==499,"Projectile did not apply one valid hit");
    auto outside=level; outside.enemies[0].x=420;
    system.load(outside,directory);
    player::WeaponFire culled; player::Inventory culledAmmo;
    culled.update({300,500,0},walls,culledAmmo,{true,true},true,0,&system);
    culled.update({300,500,1.0471975511965976},walls,culledAmmo,{},true,0,&system);
    check(system.records()[0].health==4,"An out-of-view projectile applied damage after native culling");
    // Bits set in SPT explicitly reject impact.
    write(directory/("MONS"+std::to_string(bank)+".SPT"),std::vector<unsigned char>((bank==3?16:20)*8,255));
    system.load(level,directory);
    check(!system.trace(camera,{300,500},{1,0},1000),"SPT rejected region was treated as solid");
    fixture(directory,bank);
    enemies::System slow,fast; slow.load(level,directory); fast.load(level,directory);
    player::Vitals slowVitals,fastVitals; camera={100,100,0};
    slow.update(level,camera,walls,slowVitals,0,true); fast.update(level,camera,walls,fastVitals,0,true);
    for(unsigned i=0;i<60;++i) slow.update(level,camera,walls,slowVitals,1./30,true);
    for(unsigned i=0;i<240;++i) fast.update(level,camera,walls,fastVitals,1./120,true);
    check(slow.records()[0].x==fast.records()[0].x && slow.records()[0].y==fast.records()[0].y &&
          slow.records()[0].phase==fast.records()[0].phase && slowVitals.health()==fastVitals.health(),
          "Enemy simulation depends on render FPS");
    slow.load(level,directory); fast.load(level,directory);
    slow.hit(0,4,0);fast.hit(0,4,0);
    slow.update(level,camera,walls,slowVitals,0,true);fast.update(level,camera,walls,fastVitals,0,true);
    for(unsigned i=0;i<60;++i) slow.update(level,camera,walls,slowVitals,1./30,true);
    for(unsigned i=0;i<240;++i) fast.update(level,camera,walls,fastVitals,1./120,true);
    check(slow.effects().active()==fast.effects().active() && slow.effects().slimePhase()==fast.effects().slimePhase(),
          "Death effects depend on render FPS");
    level.enemies.push_back(level.enemies[0]); level.enemies[1].x=700;
    system.load(level,directory);
    hit=system.trace({300,500,0},{300,500},{1,0},1000);
    check(hit && hit->enemy==0,"Nearest enemy ordering failed");
    system.hit(0,5,0);
    check(system.records()[1].health==4,"Damage leaked between enemies");
    camera={300,500,0};
    system.update(level,camera,walls,vitals,0,true);
    for(unsigned i=0;i<8;++i) system.update(level,camera,walls,vitals,1./18,true);
    hit=system.trace({300,500,0},{300,500},{1,0},1000);
    check(hit && hit->enemy==1,"Removed front enemy still intercepted later shots");
    level=arena(); level.monsBank=bank; level.enemies.push_back(level.enemies[0]);
    system.load(level,directory); player::Vitals concurrent;
    camera={450,500,0};
    system.update(level,camera,walls,concurrent,0,true);
    system.update(level,camera,walls,concurrent,1./18,true);
    system.update(level,camera,walls,concurrent,1./18,true);
    check(concurrent.health()==998,"Simultaneous attacks stacked independent damage reactions");
    const auto attacks=system.takeSounds();
    check(std::count(attacks.begin(),attacks.end(),9)==1 && std::count(attacks.begin(),attacks.end(),10)==1,
          "Original attack sounds did not alternate globally");
    system.hit(0,4,0); system.hit(1,4,0); system.takeSounds();
    for(unsigned i=0;i<8;++i) system.update(level,camera,walls,concurrent,1./18,true);
    const auto deaths=system.takeSounds();
    check(std::count(deaths.begin(),deaths.end(),17)==1 && system.effects().active(),
          "Simultaneous death audio did not coalesce per native pending ID");


}
void deathEffects()
{
    sprites::Bank bank;
    for(unsigned i=0;i<20;++i) bank.images.push_back({{2,2,std::vector<wall_textures::Rgb>(4)},1,2,{1,1,1,1}});
    enemies::DeathEffects effects; Camera camera{0,0,0};
    effects.begin({200,0},0,camera); effects.compose(camera,bank);
    check(effects.objects().size()==1 && effects.objects()[0].image==10 && effects.slimePhase()==0,
          "Native explosion10/near splash did not start");
    effects.step(camera,{}); effects.compose(camera,bank);
    check(effects.objects()[0].image==11 && effects.objects().size()==5 && effects.objects()[1].image==12,
          "Native phase1 did not interleave body11 and advancing fragments");
    effects.step(camera,{}); effects.compose(camera,bank);
    constexpr unsigned frames[]{12,12,13,14,15,15,13};
    check(effects.objects().size()==11,"Seven native bursts were not interleaved with four fragments");
    for(unsigned i=0;i<7;++i) check(effects.objects()[i].image==frames[i],"Native burst byte-table mapping differs");
    check(effects.objects()[7].image==13 && effects.objects()[10].image==13,
          "Fragment fade was delayed until after world bursts");
    for(unsigned i=0;i<40;++i) effects.step(camera,{});
    effects.compose(camera,bank);
    check(!effects.active() && effects.objects().empty() && !effects.slimePhase(),"Death effects did not clean up");
    effects.begin({400,0},0,camera);
    check(!effects.slimePhase(),"Splash includes excluded Chebyshev400 boundary");
    effects.begin({200,0},0,camera); effects.step(camera,{});
    effects.begin({800,0},0,camera); effects.compose(camera,bank);
    check(effects.objects()[0].x==800 && effects.objects()[0].image==10 && effects.slimePhase()==0,
          "Native single-slot replacement or independent slime timer failed");
    effects.begin({200,0},0,camera);
    const collision::World obstruction{{{200,-1000},{200,1000}}};
    effects.step(camera,obstruction); effects.compose(camera,bank);
    check(effects.objects().size()==5 && effects.objects()[1].image==16 && effects.objects()[4].image==19,
          "Native non-advancing fragment frames were changed");
    effects.reset(); check(!effects.active() && !effects.slimePhase() && effects.objects().empty(),"Effects reset failed");
}
void mons3(const std::filesystem::path& directory)
{
    fixture(directory,3);
    auto level=arena(); level.monsBank=3;
    const auto walls=collision::staticWorld(level);
    enemies::System system; player::Vitals vitals; Camera camera{300,500,0};
    system.load(level,directory);
    check(system.scene().objects[0].height==190,"MONS3 height must be 190");
    check(system.blocksInterface(level,camera,walls),"Live MONS3 did not block map");
    system.hit(0,4,0);
    system.update(level,camera,walls,vitals,0,true);
    for(unsigned i=0;i<8;++i) {
        check(system.blocksInterface(level,camera,walls),"Dying MONS3 stopped blocking map early");
        Camera attempted{450,500,0};
        system.constrainPlayer(level,camera,attempted,walls);
        check(attempted.x<400,"Dying MONS3 stopped blocking movement early");
        system.update(level,camera,walls,vitals,1./18,true);
        if(i<7) check((system.records()[0].sprite&255)==(i%2?9:5),"MONS3 death sequence differs");
    }
    check(!system.blocksInterface(level,camera,walls),"Removed MONS3 still blocks map");
    check(system.effects().objects().size()==1 && system.effects().objects()[0].image==10 &&
          system.effects().objects()[0].height==190,"MONS3 explosion body/height differs");
    system.update(level,camera,walls,vitals,1./18,true);
    check(system.effects().objects().size()==1 && system.effects().objects()[0].image==11,
          "MONS3 created body fragments");
    system.update(level,camera,walls,vitals,1./18,true);
    constexpr unsigned bursts[]{12,12,13,14,15,15,13};
    check(system.effects().objects().size()==7,"MONS3 burst count differs");
    for(unsigned i=0;i<7;++i) check(system.effects().objects()[i].image==bursts[i],"MONS3 burst sequence differs");
    for(unsigned i=0;i<7;++i) system.update(level,camera,walls,vitals,1./18,true);
    check(!system.effects().active() && system.effects().objects().empty(),"MONS3 explosion did not finish");

    // Exercise both original cycle directions without altering pursuit direction.
    for(bool reverse:{false,true}) {
        system.load(level,directory); camera.angle=reverse?3.14159265358979323846:0;
        system.update(level,camera,walls,vitals,0,true);
        system.update(level,camera,walls,vitals,1./18,true);
        check(system.records()[0].phase==(reverse?4:1),"MONS3 movement cycle direction differs");
    }
    level.enemies[0].behavior=0;
    system.load(level,directory);
    system.update(level,camera,walls,vitals,0,true);
    constexpr unsigned waiting[]{4,4,4,4,5,5,5,5,4,4,4,4,5,5,5,5,
                                 4,4,4,4,5,5,5,5,5,6,8,8,7,6,5,4};
    for(unsigned i=1;i<=32;++i) {
        system.update(level,camera,walls,vitals,1./18,true);
        check((system.records()[0].sprite&255)==waiting[i%32],"MONS3 waiting sequence differs");
    }
    level.enemies[0].behavior=2;
    system.load(level,directory);
    system.update(level,camera,walls,vitals,0,true);
    system.update(level,camera,walls,vitals,1./18,true);
    check(system.records()[0].behavior==3,"MONS3 detection did not activate pursuit");
}
void navigation()
{
    auto level=arena();
    auto& sector=level.sectors[0];
    sector.vertices={{0,0},{500,0},{500,1000},{0,1000},{1000,0},{1000,1000}};
    sector.polygons={{{0,1,2,3},0,{{1,500,500}}},{{1,4,5,2},0,{{0,500,500}}}};
    auto from=collision::locate(level,{200,500}); auto target=collision::locate(level,{800,500});
    check(from && target,"Original polygon lookup failed");
    auto point=enemies::waypoint(level,*from,{200,500},*target,{800,500});
    check(point && point->x==500 && point->y==500,"Polygon connection waypoint ignored");
    point=enemies::waypoint(level,*from,{480,500},*target,{800,500});
    check(point && point->x==800,"Enemy stopped at a reached polygon portal");
    sector.polygons[0].connections.clear();
    check(!enemies::waypoint(level,*from,{200,500},*target,{800,500}),"Missing route invented");
    // Two adjoining original-style sectors, with the sector waypoint on the shared edge.
    level=arena(); level.sectors.push_back(level.sectors[0]);
    level.sectors[1].gx=1000;
    level.sectors[0].routes={{1,1000,500}};
    level.sectors[1].routes={{0,1000,500}};
    point=enemies::waypoint(level,{0,0},{800,500},{1,0},{1500,500});
    check(point && point->x==1000,"Sector waypoint ignored");
    point=enemies::waypoint(level,{0,0},{980,500},{1,0},{1500,500});
    check(point && point->x==1500,"Sector boundary caused a permanent waypoint stop");

}
void original(const std::filesystem::path& lab)
{
    const auto level2=game_lab::loadLevel(lab,1).level;
    enemies::System mons; mons.load(level2,lab.parent_path()/"DATA");
    check(level2.monsBank==3 && mons.records().size()==12 && mons.scene().bank.images.size()==16 &&
          mons.scene().objects[0].height==190,"Original MONS3 resources differ");
    const auto target=mons.scene().objects[0];
    const Camera view{target.x-200,target.y,0};
    auto nativeHit=mons.trace(view,{view.x,view.y},{1,0},250);
    check(nativeHit && nativeHit->enemy==0,"Native MONS3 mask rejected central shot");
    mons.hit(nativeHit->enemy,4,0);
    player::Vitals nativeVitals;
    const auto nativeWalls=collision::staticWorld(level2);
    mons.update(level2,view,nativeWalls,nativeVitals,0,true);
    for(unsigned i=0;i<18;++i) {
        mons.update(level2,view,nativeWalls,nativeVitals,1./18,true);
        for(const auto& object:mons.effects().objects())
            check(object.image<16,"Native MONS3 indexed absent fragment image");
    }
    check(mons.records()[0].health==0xffff,"Native MONS3 failed to retire");
    const auto level=game_lab::loadLevel1(lab).level;
    check(level.enemies.size()==9,"Level 1 must contain nine original enemies");
    constexpr unsigned sectors[]{3,5,6,7,14,12,11,10,12};
    constexpr unsigned xs[]{300,700,300,100,500,700,900,300,700};
    constexpr unsigned ys[]{100,500,300,700,500,700,700,200,500};
    for (unsigned i=0;i<9;++i) {
        const auto& e=level.enemies[i];
        check(e.health==4 && e.state==6 && e.sector==sectors[i] && e.x==xs[i] && e.y==ys[i],
              "Original enemy record differs");
    }
    enemies::System system; system.load(level,lab.parent_path()/"DATA");
    check(system.scene().objects.size()==9 && system.scene().bank.images.size()==20,"MONS1 load failed");
    check(level.sectors[2].routes.size()==3 && level.sectors[2].routes[0].sector==1 &&
          level.sectors[2].routes[0].x==3500 && level.sectors[2].routes[0].y==1700,
          "Original sector route table differs");
    for(unsigned i=0;i<9;++i) {
        const auto& object=system.scene().objects[i];
        const auto location=collision::locate(level,{object.x,object.y},sectors[i]);
        check(location && location->sector==sectors[i],"Original enemy is outside its sector/polygon");
    }
    const Camera player{4100,2050,1.5707963267948966};
    auto hit=system.trace(player,{player.x,player.y},{0,1},1000);
    check(hit && hit->enemy==0,"Native MONS1 mask does not admit the central shot");
    player::Vitals vitals; const auto walls=collision::staticWorld(level);
    system.update(level,player,walls,vitals,0,true);
    for(unsigned i=0;i<18;++i) system.update(level,player,walls,vitals,1./18,true);
    check(vitals.health()<1000,"Native enemy/sector/animation did not reach its attack");

}
}
int main(int argc,char** argv)
{
    try {
        if (argc<2) return 1;
        placement(argv[1]); otherBanks(argv[1]); allBanks(argv[1],true); combat(argv[1]); combat(argv[1],3); mons3(argv[1]); deathEffects(); navigation();
        if (argc>2) { original(argv[2]); allBanks(std::filesystem::path(argv[2]).parent_path()/"DATA",false); }
        std::cout<<"Enemy placement/resource tests passed\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
