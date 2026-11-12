#include "enemy_system.h"
#include "enemy_profiles.h"
#include "sprite_renderer.h"
#include "weapon_fire.h"
#include "enemy_navigation.h"
#include "objet_collision.h"
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
void objetCollisions(const std::filesystem::path& directory)
{
    auto level=arena(); level.objetBank=1;
    level.sectors[0].vertices.push_back({500,500});
    level.sectors[0].entitiesA={{4,0x5004,256,0}};
    const auto resolve=[&](collision::Point before, collision::Point candidate,
                           unsigned angle, collision::Point expected, unsigned expectedAngle) {
        auto heading=std::uint16_t(angle);
        const auto result=collision::constrainObjets(level,0,before,candidate,nullptr,&heading);
        check(result.x==expected.x && result.y==expected.y && heading==expectedAngle,
              "OBJET enemy bounds/contact angle differs from DOS");
    };
    resolve({400,500},{430,500},0,{420,500},80);
    resolve({400,500},{430,500},700,{420,500},620);
    resolve({600,500},{570,500},360,{580,500},440);
    resolve({600,500},{570,500},340,{580,500},260);
    resolve({500,400},{500,430},180,{500,420},260);
    resolve({500,400},{500,430},160,{500,420},80);
    resolve({500,600},{500,570},540,{500,580},620);
    resolve({500,600},{500,570},520,{500,580},440);
    resolve({500,600},{500,570},700,{500,580},60); // Wrap after +80.
    resolve({400,400},{430,430},90,{430,420},10); // Y before X.
    resolve({400,500},{430,500},360,{420,500},440); // Signed antipodal ties.
    resolve({600,500},{570,500},0,{580,500},640);
    resolve({500,400},{500,430},540,{500,420},460);
    resolve({500,600},{500,570},180,{500,580},260);
    resolve({600,500},{420,500},360,{580,500},440); // Lower inclusive.
    resolve({400,500},{580,500},0,{580,500},0); // Upper exclusive.
    resolve({500,600},{500,420},540,{500,580},620);
    resolve({500,400},{500,580},180,{500,580},180);
    resolve({500,500},{501,501},0,{501,501},0); // No invented depenetration.
    resolve({400,500},{600,500},0,{600,500},0); // Candidate point, not sweep.

    for (unsigned bank : {1u,5u}) {
        fixture(directory,bank);
        level.monsBank=bank;
        for (unsigned type : {0x5004u,0x4004u,0x7004u,0x5007u}) {
            level.sectors[0].entitiesA[0].type=std::uint16_t(type);
            level.enemies[0].x=400; level.enemies[0].angle=0;
            enemies::System system; system.load(level,directory);
            player::Vitals vitals; const Camera camera{900,500,0};
            system.update(level,camera,{},vitals,0,true);
            system.update(level,camera,{},vitals,1./18,true);
            const auto& e=system.records()[0];
            const bool solid=type==0x5004;
            check(e.x==(solid?420:bank==5?450:430) && e.y==500 && e.angle==(solid?80:0),
                  "Enemy normal/MONS5 movement ignored OBJET solidity or angular response");
        }
        // Current image, not the initial entity image or rendered dimensions.
        level.sectors[0].entitiesA[0].type=0x5007;
        sprites::Scene scene; scene.objetBank=1;
        scene.objects.push_back({500,500,256,1,21,0x5007,0,0,0});
        level.enemies[0].x=340;
        enemies::System system; system.load(level,directory);
        player::Vitals vitals; const Camera camera{900,500,0};
        system.update(level,camera,{},vitals,0,true,{}, {}, {}, &scene);
        system.update(level,camera,{},vitals,1./18,true,{}, {}, {}, &scene);
        check(system.records()[0].x==350 && system.records()[0].angle==80,
              "Enemy ignored current OBJET image extent");
        scene.objects[0].image=7; scene.objects[0].height=10000;
        system.load(level,directory);
        system.update(level,camera,{},vitals,0,true,{}, {}, {}, &scene);
        system.update(level,camera,{},vitals,1./18,true,{}, {}, {}, &scene);
        check(system.records()[0].x==(bank==5?390:370) && system.records()[0].angle==0,
              "Traversable current OBJET image used visual size or stale extent");
    }
    // MONS5 hurt recoil moves backwards and still uses the facing angle response.
    level.sectors[0].entitiesA[0].type=0x5004;
    level.enemies[0].x=600;
    for (unsigned health : {4u,20u}) {
        level.enemies[0].health=std::uint16_t(health);
        enemies::System system; system.load(level,directory);
        system.hit(0,4,3.14159265358979323846); // Facing east, recoil 30 west.
        player::Vitals vitals; const Camera camera{900,500,0};
        system.update(level,camera,{},vitals,0,true);
        system.update(level,camera,{},vitals,1./18,true);
        check(system.records()[0].x==580 && system.records()[0].angle==640,
              "MONS5 hurt/death recoil bypassed OBJET or used travel direction for angle");
    }
}
void sectorVisibility(const std::filesystem::path& directory)
{
    auto level=arena();
    level.sectors.push_back(level.sectors[0]); level.sectors[1].gx=1000;
    level.sectors[0].neighbors={1}; level.sectors[1].neighbors={0};
    level.connectionStates={0x0400};
    std::vector<doors::Door> dynamic(1);
    auto& door=dynamic[0]; door.stateIndex=0; door.originalState=0x0400;
    const auto connected=[&]() -> bool { return doors::connectedSectors(level,1,dynamic)[0]; };
    check(!connected(),"Closed locked connection enabled");
    door.originalState=0;
    check(!connected(),"Unlocking enabled a closed connection");
    door.state=doors::State::Opening;
    check(connected(),"Opening at zero progress did not enable connection");
    door.state=doors::State::Open; door.progress=1;
    check(connected(),"Open connection disabled");
    door.state=doors::State::Closing; door.progress=.5;
    check(connected(),"Closing positive progress disabled connection");
    door.progress=0;
    check(!connected(),"Closing zero progress enabled connection");
    door.state=doors::State::Closed;
    check(!connected(),"Finished closure enabled connection");
    level.connectionStates[0]=0x0100;
    check(doors::connectedSectors(level,1,{})==std::vector<bool>({true,true}),
          "Original connection or cyclic traversal failed");
    check(!connected(),"Original word overrode dynamic closed door");
    level.connectionStates[0]=0x0400;
    check(!doors::connectedSectors(level,1,{})[0],"Lock bit enabled original connection");
    level.sectors[1].neighbors={99,0}; // Invalid neighbor and missing connection wall.
    level.sectors[1].walls.resize(1);
    check(doors::connectedSectors(level,1,{})==std::vector<bool>({false,true}),
          "Invalid neighbor traversed");
    level.sectors[1].neighbors={0}; level.sectors[1].walls[0].extra=99;
    check(!connected(),"Invalid connection state traversed");
    check(doors::connectedSectors(level,99,dynamic)==std::vector<bool>({false,false}),
          "Invalid player sector traversed");
    level.sectors[1].walls[0].extra=0;
    const Camera camera{1500,500,0}; // Enemy is behind, in the other sector.
    const auto walls=collision::staticWorld(level); // Also retains opaque separating walls.
    for (unsigned bank : {1u,5u}) {
        fixture(directory,bank); level.monsBank=bank;
        level.enemies[0].behavior=2;
        enemies::System system; system.load(level,directory); player::Vitals vitals;
        door.state=doors::State::Closed; door.progress=0;
        check(!system.blocksInterface(level,camera,dynamic),"Closed door did not isolate enemy");
        system.update(level,camera,walls,vitals,0,true,{}, {},dynamic);
        system.update(level,camera,walls,vitals,1./18,true,{}, {},dynamic);
        check(system.records()[0].behavior==2,"Isolated enemy activated");
        door.state=doors::State::Opening;
        check(system.blocksInterface(level,camera,dynamic),"Connected enemy behind player did not block map");
        system.update(level,camera,walls,vitals,1./18,true,{}, {},dynamic);
        check(system.records()[0].behavior==3,"Opening connection did not activate behavior 2");
        level.enemies[0].health=0xffff; system.load(level,directory);
        check(!system.blocksInterface(level,camera,dynamic),"Removed connected enemy blocked map");
        level.enemies[0].health=4;
    }
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
        check(vitals.health()<1000,"Enabled bank failed to attack");
        if(bank==6)
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
        const unsigned expectedStep=bank==6?0:(bank==5?50:(bank==9?10:30));
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
    check(system.blocksInterface(level,camera),"Live MONS3 did not block map");
    system.hit(0,4,0);
    system.update(level,camera,walls,vitals,0,true);
    for(unsigned i=0;i<8;++i) {
        check(system.blocksInterface(level,camera),"Dying MONS3 stopped blocking map early");
        Camera attempted{450,500,0};
        system.constrainPlayer(level,camera,attempted,walls);
        check(attempted.x<400,"Dying MONS3 stopped blocking movement early");
        system.update(level,camera,walls,vitals,1./18,true);
        if(i<7) check((system.records()[0].sprite&255)==(i%2?9:5),"MONS3 death sequence differs");
    }
    check(!system.blocksInterface(level,camera),"Removed MONS3 still blocks map");
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
void sharedMovement(const std::filesystem::path& directory)
{
    for (unsigned bank : {1u,2u,3u,4u,7u,8u,9u}) {
        fixture(directory,bank);
        auto level=arena(); level.monsBank=bank;
        level.sectors.push_back(level.sectors[0]); level.sectors[1].gx=1000;
        level.sectors[0].walls.erase(level.sectors[0].walls.begin()+1);
        level.sectors[1].walls.pop_back();
        level.sectors[0].routes={{1,1000,500}};
        level.sectors[1].routes={{0,1000,500}};
        level.enemies[0].x=200; level.enemies[0].homeX=100;
        level.enemies[0].angle=0;
        const auto walls=collision::staticWorld(level);
        enemies::System system; player::Vitals vitals;
        auto run=[&](Camera camera,unsigned ticks) {
            system.update(level,camera,walls,vitals,0,true);
            for(unsigned n=0;n<ticks;++n) {
                vitals=player::Vitals{};
                system.update(level,camera,walls,vitals,1./18,true);
            }
        };
        level.enemies[0].behavior=1; system.load(level,directory);
        run({800,500,0},10);
        check(system.scene().objects[0].x>250,"Behavior 1 failed to pursue in same sector");
        run({800,500,0},80);
        check(vitals.health()<1000 || system.records()[0].state==2,
              "Behavior 1 failed to attack in same polygon");
        system.load(level,directory); run({1600,500,0},40);
        check(system.scene().objects[0].x<150 && system.records()[0].sector==0,
              "Behavior 1 did not return home with player outside sector");
        for(unsigned behavior : {2u,3u,4u}) {
            level.enemies[0].behavior=behavior; system.load(level,directory);
            run({1600,500,0},40); // No enabled sector connection; enemy remains dormant.
            if (behavior==2) {
                check(system.records()[0].behavior==2 && system.scene().objects[0].x<150,
                      "Dormant behavior 2 pursued without activation");
                run({800,500,0},10);
                check(system.records()[0].behavior==3 && system.scene().objects[0].x>200,
                      "Behavior 2 failed same-sector activation and movement");
            }
            run({1600,500,0},150);
            check(system.records()[0].sector==1 && system.scene().objects[0].x>1400,
                  "Active behavior failed real movement across a routed sector boundary");
        }
        level.enemies[0].behavior=2; system.load(level,directory);
        system.hit(0,0,0); run({1600,500,0},150);
        check(system.records()[0].behavior==3 && system.records()[0].sector==1,
              "Hit failed to activate cross-sector pursuit");
        level.sectors[1].neighbors={0}; level.connectionStates={0x0100};
        system.load(level,directory); run({1600,500,3.141592653589793},150);
        check(system.records()[0].behavior==3 && system.records()[0].sector==1,
              "Connected behavior 2 failed to activate and cross sectors");
        level.sectors[0].routes.clear(); level.enemies[0].behavior=4;
        system.load(level,directory); run({1600,500,0},40);
        check(system.scene().objects[0].x==200,"Enemy invented missing sector route");
    }
    // A U-shaped sector: returning directly home pushes into the inner wall.
    auto level=arena(); auto& s=level.sectors[0];
    s.vertices={{0,0},{400,0},{400,600},{600,600},{600,0},{1000,0},
                {1000,1000},{600,1000},{400,1000},{0,1000}};
    s.polygons={{{0,1,2,8,9},0,{{1,400,800}}},
                {{2,3,7,8},0,{{0,400,800},{2,600,800}}},
                {{3,4,5,6,7},0,{{1,600,800}}}};
    s.walls.clear();
    for(unsigned i=0;i<10;++i) s.walls.push_back({std::uint16_t(i),std::uint16_t((i+1)%10),1,0});
    level.enemies[0].x=200; level.enemies[0].y=200;
    level.enemies[0].homeX=800; level.enemies[0].homeY=200;
    const auto walls=collision::staticWorld(level);
    for(unsigned behavior : {1u,2u,3u,4u}) {
        level.enemies[0].behavior=behavior;
        enemies::System system; system.load(level,directory); player::Vitals vitals;
        const Camera camera=behavior<3 ? Camera{2000,2000,0} : Camera{800,200,0};
        system.update(level,camera,walls,vitals,0,true);
        for(unsigned n=0;n<180;++n) {
            vitals=player::Vitals{}; system.update(level,camera,walls,vitals,1./18,true);
            const auto& p=system.scene().objects[0];
            check(collision::locate(level,{p.x,p.y}).has_value(),"Movement left navigable polygons");
        }
        const auto& p=system.scene().objects[0];
        check(p.x>700 && p.y<300,"Enemy stuck on wall instead of following polygon route");
    }
}
void mons5(const std::filesystem::path& directory)
{
    fixture(directory,5);
    auto level=arena(); level.monsBank=5;
    level.sectors[0].vertices={{0,0},{4000,0},{4000,1000},{0,1000}};
    auto& enemy=level.enemies[0]; enemy.angle=0;
    const Camera far{3500,500,3.141592653589793};
    auto walls=collision::staticWorld(level);
    enemies::System system; player::Vitals vitals;
    auto tick=[&](const Camera& camera) { system.update(level,camera,walls,vitals,1./18,true); };
    auto reset=[&]() { system.load(level,directory); vitals.reset(); system.update(level,far,walls,vitals,0,true); };
    reset();
    constexpr unsigned images[]{3,3,4,4,4,3,3,3,2,1,0,1,2,3};
    constexpr unsigned heights[]{30,57,77,88,88,77,57,30,0,0,0,0,0,0};
    for(unsigned n=0;n<14;++n) {
        tick(far); const auto& e=system.records()[0];
        check((e.sprite&255)==images[n] && (e.elevation&255)==heights[n],
              "MONS5 native jump phase/frame/height differs");
        check(e.x==500+50*std::min(n+1,9u)+25*(n>=9?n-8:0),"MONS5 jump horizontal speed differs");
        check(system.scene().objects[0].z==heights[n],"MONS5 elevation never reached rendered position");
    }
    // Reverse directional cycle is native and wraps signed phases below -120.
    reset(); tick({3500,500,0});
    check(system.records()[0].phase==std::uint16_t(-40) && system.records()[0].x==550,
          "MONS5 reverse jump phase differs");
    for(int n=0;n<4;++n) tick({3500,500,0});
    check(system.records()[0].phase==360 && system.records()[0].x==650,
          "MONS5 reverse cycle did not use ground half-speed");
    enemy.phase=140; reset(); tick(far);
    check((system.records()[0].elevation&255)==90,"MONS5 sine apex differs");
    enemy.phase=0;
    // Attack has no horizontal travel; each phase >60 refreshes the shared reaction.
    enemy.state=2; reset();
    constexpr unsigned attackImages[]{6,7,8,8,8,8,8,8};
    for(unsigned n=0;n<8;++n) {
        tick({550,500,0}); const auto& e=system.records()[0];
        check(e.state==2 && e.phase==(n+1)*40 && (e.sprite&255)==attackImages[n],
              "MONS5 attack sequence differs");
        check(e.x==500 && e.y==500,"MONS5 attack incorrectly travelled horizontally");
        check(vitals.health()==1000-int(n)*2,"MONS5 attack damage timing differs");
    }
    const auto sounds=system.takeSounds();
    check(std::find(sounds.begin(),sounds.end(),9)!=sounds.end() &&
          std::find(sounds.begin(),sounds.end(),10)!=sounds.end(),"MONS5 attack sounds missing");
    for(unsigned n=0;n<8;++n) tick({4500,500,0});
    check(vitals.health()==972 && !(system.records()[0].elevation&255),
          "MONS5 attack tail or landing differs");
    // Hit during flight halves altitude, knocks back with strong weapons, then recovers.
    enemy.state=0; enemy.health=20; reset();
    for(unsigned n=0;n<4;++n) tick(far);
    const auto airborne=system.records()[0];
    system.hit(0,4,3.141592653589793); tick(far);
    check(system.records()[0].state==4 && (system.records()[0].sprite&255)==9 &&
          (system.records()[0].elevation&255)==44 && system.records()[0].x==airborne.x-30,
          "MONS5 airborne hurt/knockback differs");
    tick(far);
    check(system.records()[0].state!=4 && (system.records()[0].elevation&255)<=30,
          "MONS5 did not recover from airborne hit");
    // Hurt interrupts an attack too; death halves height and retires at eight ticks.
    enemy.state=2; enemy.phase=120; enemy.elevation=77; enemy.health=4; reset();
    system.hit(0,0,0); tick({550,500,0});
    check(vitals.health()==1000 && system.records()[0].state==4,
          "Interrupted MONS5 attack still damaged player");
    system.hit(0,4,0);
    for(unsigned n=0;n<8;++n) {
        tick(far);
        check((system.records()[0].sprite&255)==(n%2?9u:5u),"MONS5 death frames differ");
    }
    check(system.records()[0].health==0xffff && system.scene().objects.empty() && system.effects().active(),
          "MONS5 death failed to retire/start effects");
    enemy.state=0; enemy.phase=0; enemy.elevation=0; enemy.health=4;
    // An airborne actor must still collide with a closed wall.
    walls.push_back({{700,0},{700,1000}}); reset();
    bool lifted=false;
    for(unsigned n=0;n<40;++n) {
        tick(far); const auto& e=system.records()[0]; lifted|=(e.elevation&255)>0;
        check(e.x<=670 && collision::locate(level,{double(e.x),double(e.y)}).has_value(),
              "MONS5 crossed a wall or left navigable polygons while jumping");
    }
    check(lifted && system.records()[0].x==670,"MONS5 collision probe never reached wall");
    walls=collision::staticWorld(level);
    enemies::System slow,fast; player::Vitals sv,fv;
    slow.load(level,directory); fast.load(level,directory);
    slow.update(level,far,walls,sv,0,true); fast.update(level,far,walls,fv,0,true);
    for(unsigned n=0;n<60;++n) slow.update(level,far,walls,sv,1./30,true);
    for(unsigned n=0;n<240;++n) fast.update(level,far,walls,fv,1./120,true);
    check(slow.records()[0].x==fast.records()[0].x && slow.records()[0].phase==fast.records()[0].phase &&
          slow.records()[0].elevation==fast.records()[0].elevation && sv.health()==fv.health(),
          "MONS5 movement depends on rendering FPS");
    const auto paused=slow.records()[0]; slow.update(level,far,walls,sv,1,false);
    slow.update(level,far,walls,sv,1,true);
    check(slow.records()[0].x==paused.x && slow.records()[0].phase==paused.phase,
          "MONS5 advanced during pause/resume");
    // Real cross-sector movement, including a grounded half-speed phase near the portal.
    level=arena(); level.monsBank=5; level.enemies[0].angle=0;
    level.sectors.push_back(level.sectors[0]); level.sectors[1].gx=1000;
    level.sectors[0].walls.erase(level.sectors[0].walls.begin()+1); level.sectors[1].walls.pop_back();
    level.sectors[0].routes={{1,1000,500}}; level.sectors[1].routes={{0,1000,500}};
    walls=collision::staticWorld(level);
    for(unsigned behavior : {1u,2u,3u,4u}) {
        level.enemies[0].behavior=behavior; reset();
        if(behavior==2) system.hit(0,0,0);
        for(unsigned n=0;n<90;++n) tick({1600,500,0});
        if(behavior==1) check(system.records()[0].sector==0 && system.records()[0].x==500,
                              "MONS5 behavior 1 left its home sector");
        else check(system.records()[0].sector==1 && system.scene().objects[0].x>1400 && vitals.health()<1000,
                   "MONS5 failed routed pursuit and attack across sectors");
    }
}
void originalMons5(const std::filesystem::path& lab)
{
    const auto original=game_lab::loadLevel(lab,2).level;
    check(original.monsBank==5 && original.enemies.size()==18,"Native level 3 MONS5 placements differ");
    const auto walls=collision::staticWorld(original);
    for(unsigned index=0;index<original.enemies.size();++index) {
        auto level=original; level.enemies={original.enemies[index]};
        enemies::System system; system.load(level,lab.parent_path()/"DATA");
        check(system.scene().bank.images.size()==16 && system.scene().objects[0].height==180,
              "Native MONS5 resources/profile differ");
        const auto start=system.scene().objects[0];
        const auto from=collision::locate(level,{start.x,start.y},level.enemies[0].sector);
        std::optional<Camera> camera;
        for(int dx=-400;dx<=400 && !camera;dx+=50) for(int dy=-400;dy<=400 && !camera;dy+=50) {
            if(std::max(std::abs(dx),std::abs(dy))<150) continue;
            const collision::Point goal{start.x+dx,start.y+dy};
            const auto to=collision::locate(level,goal,from->sector);
            if(!to || to->sector!=from->sector || to->polygon!=from->polygon ||
               collision::rayDistance(walls,{start.x,start.y},{double(dx),double(dy)},1)) continue;
            if(std::any_of(walls.begin(),walls.end(),[&](const auto& wall) {
                return collision::distanceToSegment(wall,goal)<40;
            })) continue;
            camera=Camera{goal.x,goal.y,std::atan2(-dy,-dx)};
        }
        check(camera.has_value(),("No original MONS5 same-polygon approach probe: "+std::to_string(index)).c_str());
        player::Vitals vitals; system.update(level,*camera,walls,vitals,0,true);
        bool horizontal=false,vertical=false,attacked=false;
        for(unsigned tick=0;tick<120;++tick) {
            system.update(level,*camera,walls,vitals,1./18,true);
            const auto object=system.scene().objects[0];
            horizontal|=std::hypot(object.x-start.x,object.y-start.y)>20;
            vertical|=object.z>0; attacked|=system.records()[0].state==2;
            check(collision::locate(level,{object.x,object.y}).has_value(),"Native MONS5 left polygon bounds");
        }
        check(horizontal && vertical && attacked && vitals.health()<1000,
              ("Native MONS5 movement/attack failed: "+std::to_string(index)).c_str());
        system.hit(0,0,0);
        check(system.records()[0].state==4,"Native MONS5 hit did not interrupt action");
        for(unsigned hit=0;hit<20 && system.records()[0].state!=5;++hit) system.hit(0,4,0);
        for(unsigned tick=0;tick<8;++tick) system.update(level,*camera,walls,vitals,1./18,true);
        check(system.records()[0].health==0xffff && system.scene().objects.empty(),
              "Native MONS5 placement did not retire after death");
    }
}
void original(const std::filesystem::path& lab)
{
    const auto level2=game_lab::loadLevel(lab,1).level;
    enemies::System mons; mons.load(level2,lab.parent_path()/"DATA");
    check(level2.monsBank==3 && mons.records().size()==12 && mons.scene().bank.images.size()==16 &&
          mons.scene().objects[0].height==190,"Original MONS3 resources differ");
    // Native level 2 behavior-1 placements must move, not merely change frames.
    for (unsigned i=0;i<level2.enemies.size();++i) {
        if (level2.enemies[i].behavior!=1) continue;
        auto single=level2; single.enemies={level2.enemies[i]};
        enemies::System walker; walker.load(single,lab.parent_path()/"DATA");
        const auto start=walker.scene().objects[0];
        const auto from=collision::locate(single,{start.x,start.y},single.enemies[0].sector);
        const auto walls=collision::staticWorld(single);
        bool tested=false;
        std::vector<collision::Point> candidates;
        for(int dx=-300;dx<=300;dx+=50) for(int dy=-300;dy<=300;dy+=50)
            if (std::max(std::abs(dx),std::abs(dy))>=150) candidates.push_back({double(dx),double(dy)});
        for (collision::Point delta : candidates) {
            const Camera camera{start.x+delta.x,start.y+delta.y,0};
            const auto to=collision::locate(single,{camera.x,camera.y},from->sector);
            if (!to || to->sector!=from->sector || to->polygon!=from->polygon ||
                collision::rayDistance(walls,{start.x,start.y},delta,1)) continue;
            player::Vitals vitals; walker.update(single,camera,walls,vitals,0,true);
            for(unsigned tick=0;tick<12;++tick) walker.update(single,camera,walls,vitals,1./18,true);
            const auto end=walker.scene().objects[0];
            check(std::hypot(end.x-start.x,end.y-start.y)>20,
                  "Native MONS3 behavior 1 animated without displacement");
            tested=true; break;
        }
        check(tested,"No same-polygon movement probe for native MONS3 placement");
    }
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
        if (argc==3 && std::string(argv[2])=="--objets") {
            objetCollisions(argv[1]);
            std::cout << "Enemy OBJET collision tests passed\n";
            return 0;
        }
        sectorVisibility(argv[1]); placement(argv[1]); otherBanks(argv[1]); allBanks(argv[1],true); combat(argv[1]); combat(argv[1],3); mons3(argv[1]); deathEffects(); navigation(); sharedMovement(argv[1]); mons5(argv[1]); objetCollisions(argv[1]);
        if (argc>2) { originalMons5(argv[2]); original(argv[2]); allBanks(std::filesystem::path(argv[2]).parent_path()/"DATA",false); }
        std::cout<<"Enemy placement/resource tests passed\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
