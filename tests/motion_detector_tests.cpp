#include "extremis/gameplay/enemies/motion_detector.h"
#include "extremis/gameplay/enemies/enemy_system.h"
#include "extremis/gameplay/player/player_inventory.h"
#include "extremis/resources/game_lab.h"
#include "extremis/ui/original_hud.h"
#include "extremis/world/door.h"
#include "extremis/world/original_angle_tables.h"
#include "vortex/core/graphics/color.h"
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <cmath>
namespace {
void check(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
game_lab::EnemyRecord enemy(int dx,int dy)
{
    return {4,2,6,0,0,std::uint16_t(3000+dx),std::uint16_t(3000+dy),0,0,0,0x8000,0,0,0};
}
void scanRules()
{
    game_lab::Level level{}; level.sectors.resize(1);
    const Camera camera{3000,3000,0};
    auto reading=detector::scan(level,{enemy(640,0)},camera,4,true);
    check(reading.count==1 && reading.contacts[0].x==24 && reading.contacts[0].y==188 &&
          reading.contacts[0].intensity==0,"Native forward coordinates/sweep intensity differ");
    check(detector::scan(level,{enemy(640,0)},camera,0,true).count==0,"Native sweep gap ignored");
    reading=detector::scan(level,{enemy(0,640)},{3000,3000,1.5707963267948966},4,true);
    check(reading.count==1 && reading.contacts[0].x==24 && reading.contacts[0].y==188,"Orientation did not rotate contacts");
    reading=detector::scan(level,{enemy(640,0)},{3000,3000,vortex::math::pi/2},7,true);
    check(reading.count==1 && reading.contacts[0].x==34 && reading.contacts[0].y==198,"Native X sign was inverted");
    check(detector::scan(level,{enemy(2048,0)},camera,4,true).count==0,"Forward32 bound included");
    check(detector::scan(level,{enemy(2047,0)},camera,4,true).count==1,"Forward31 bound excluded");
    check(detector::scan(level,{enemy(640,-1728)},camera,7,true).count==0,"Lateral27 bound included");
    auto stationary=enemy(640,0);
    check(detector::scan(level,{stationary},camera,4,true).count==1,"Stationary enemy was excluded");
    stationary.health=0; stationary.state=5;
    check(detector::scan(level,{stationary},camera,4,true).count==1,"Dying enemy excluded before native removal");
    stationary.health=0xffff;
    check(detector::scan(level,{stationary},camera,4,true).count==0,"Removed enemy still detected");
    check(!detector::scan(level,{enemy(640,0)},camera,4,false).phase,"Off detector animated");
    std::vector<game_lab::EnemyRecord> many(11,enemy(640,0));
    check(detector::scan(level,many,camera,4,true).count==10,"Native ten-contact cap changed");
    many.assign(10,enemy(0,640));
    many.push_back(enemy(128,0));
    check(detector::scan(level,many,camera,4,true).count==0,
          "A nearer, illuminated eleventh record bypassed the native pre-sweep cap");
    // No geometry/door or velocity dependency exists in the original scan.
    level.sectors[0].walls={{0,1,1,0}};
    check(detector::scan(level,{enemy(640,0)},camera,4,true).count==1,"Wall topology affected detector");
}
void battery()
{
    using Mode=player::BatteryMode;
    player::Inventory inv;
    check(inv.batteryCharge()==-1 && !inv.setBatteryMode(Mode::Radar) &&
          !inv.setBatteryMode(Mode::Optics),"Absent battery powered a device");
    std::uint16_t item=1;
    check(inv.takeSupply(item)==player::Transfer::Taken && item==0xffff && inv.batteryCharge()==160,
          "Original cabinet-to-PIT battery conversion failed");
    inv.updateBattery(100);
    check(inv.batteryCharge()==160,"Inactive mode drained battery");
    check(inv.toggleDetector(),"Charged radar did not activate");
    inv.updateBattery(80);
    check(inv.batteryCharge()==80 && inv.detectorEnabled(),"Radar consumption differs from the PIT delta");
    check(inv.setBatteryMode(Mode::Optics) && !inv.detectorEnabled() &&
          inv.batteryMode()==Mode::Optics && inv.batteryCharge()==80,"Direct switch changed charge or left radar enabled");
    inv.updateBattery(79);
    check(inv.batteryCharge()==1 && inv.batteryMode()==Mode::Optics,"Optics drained at a different rate");
    inv.updateBattery(1);
    check(inv.batteryCharge()==0 && inv.batteryMode()==Mode::Off,"Exact exhaustion did not switch optics off");

    std::uint16_t fresh=900;
    check(inv.takeSupply(fresh)==player::Transfer::Exchanged && fresh==0 &&
          inv.batteryCharge()==144000 && inv.batteryMode()==Mode::Off,"Empty battery exchange was not exact");
    inv.setBatteryMode(Mode::Optics);
    inv.updateBattery(321);
    std::uint16_t smaller=80;
    check(inv.takeSupply(smaller)==player::Transfer::Capacity && smaller==80 &&
          inv.batteryCharge()==143679,"Unconfirmed exchange changed inventory or cabinet");
    check(inv.takeSupply(smaller,true)==player::Transfer::Exchanged && smaller==897 &&
          inv.batteryCharge()==12800 && inv.batteryMode()==Mode::Optics,"Charged exchange did not conserve floored charge");
    check(inv.takeSupply(smaller,true)==player::Transfer::Exchanged && smaller==80 &&
          inv.batteryCharge()==143520,"Reverse exchange restored spent or rounded charge");
}
void original(const char* path)
{
    std::ifstream file(path,std::ios::binary);
    const std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(file),{}};
    check(bytes.size()>0x3da5,"Cannot read native EXE");
    for(unsigned i=0;i<720;++i) {
        const auto word=[&](unsigned offset) {
            return std::int16_t(unsigned(bytes[offset+2*i])|(unsigned(bytes[offset+2*i+1])<<8));
        };
        check(detector::cosine[i]==word(0x3164),"Radar Q14 cosine differs from EXE");
        check(detector::cosine[(i+540)%720]==word(0x3704),"Radar Q14 sine differs from EXE");
    }
    for(unsigned i=0;i<257;++i) check(detector::arctangent[i]==bytes[0x3ca4+i],"Radar atan table differs from EXE");
}

void originalLevel(const std::filesystem::path& lab)
{
    const auto level=game_lab::loadLevel(lab,1).level;
    const auto directory=lab.parent_path()/"DATA";
    enemies::System system;
    system.load(level,directory);
    const auto resources=hud::load(directory,level.palBank);
    const hud::Renderer renderer(resources);
    const auto color=[&](unsigned index) {
        const auto& c=resources.palette[index];
        return vortex::packOpaqueRgb(c.r,c.g,c.b);
    };
    const auto checkPixels=[&](const detector::Reading& reading) {
        hud::State state;
        state.detector=reading;
        std::vector<std::uint32_t> pixels(hud::width*hud::height);
        check(renderer.draw(pixels,state),"Native radar HUD draw failed");
        const auto& sweep=resources.casque[5+*reading.phase];
        for (int y=171;y<200;++y) for (int x=0;x<46;++x) {
            auto index=sweep.indices[(y-171)*46+x];
            if (reading.count) {
                const auto& contact=reading.contacts[0];
                const auto& marker=resources.casque[13+contact.intensity];
                const int sx=x-contact.x+marker.width/2, sy=y-contact.y+marker.height/2;
                if (sx>=0 && sx<marker.width && sy>=0 && sy<marker.height &&
                    marker.indices[sy*marker.width+sx])
                    index=marker.indices[sy*marker.width+sx];
            }
            check(pixels[y*hud::width+x]==color(index),"Native CASQUE marker displaced from its world projection");
        }
    };

    // The render scene is the world-position reference. Exercise each DOS
    // cardinal from four valid viewpoints around the unmodified stock spawn.
    constexpr collision::Point forward[]{{1,0},{0,1},{-1,0},{0,-1}};
    const auto target=system.scene().objects.at(0);
    const std::vector<game_lab::EnemyRecord> first{system.records().at(0)};
    for (const auto offset : forward) {
        Camera camera{target.x-128*offset.x,target.y-128*offset.y,0};
        const auto location=collision::locate(level,{camera.x,camera.y},first[0].sector);
        check(location && location->sector==first[0].sector,"Original radar viewpoint is outside the enemy sector");
        for (unsigned turn=0;turn<4;++turn) {
            camera.angle=turn*vortex::math::pi/2;
            const auto aim=forward[turn];
            const double ahead=offset.x*aim.x+offset.y*aim.y;
            const double right=offset.x*aim.y-offset.y*aim.x;
            const int centerPhase=ahead>0 ? 4 : ahead<0 ? 10 : right>0 ? 7 : 1;
            for (unsigned phase=0;phase<8;++phase) {
                const auto reading=detector::scan(level,first,camera,phase,true);
                const auto fade=unsigned(std::abs(int(phase)-centerPhase));
                check(reading.count==unsigned(fade<=2),"Native sweep differs for a real cardinal enemy");
                if (reading.count) {
                    const auto& contact=reading.contacts[0];
                    check(contact.x==24+2*right && contact.y==198-2*ahead && contact.intensity==fade,
                          "Native radar projection differs from the real enemy position");
                }
                checkPixels(reading);
            }
        }
    }

    // Stock enemy 11 pursues through the sector 11→10 connection. Track the
    // rendered position on both sides of the crossing, with live local XY.
    Camera camera{400,500,vortex::math::pi/2};
    camera.sector=10;
    const auto walls=collision::staticWorld(level);
    const auto dynamicDoors=doors::closedDoors(level);
    const auto objects=sprites::loadScene(level,directory);
    player::Vitals vitals;
    system.update(level,camera,walls,vitals,0,true,{}, {},dynamicDoors,&objects);
    unsigned previousSector=system.records().at(11).sector;
    std::optional<detector::Contact> previous;
    bool moved=false,crossed=false;
    for (unsigned tick=1;tick<=25;++tick) {
        system.update(level,camera,walls,vitals,1./18,true,{}, {},dynamicDoors,&objects);
        const auto& record=system.records().at(11);
        const auto& world=system.scene().objects.at(11);
        const int x=24+int(std::floor((world.x-camera.x)/64));
        const int y=198-int(std::floor((world.y-camera.y)/64));
        const std::vector<game_lab::EnemyRecord> live{record};
        std::optional<detector::Contact> current;
        for (unsigned phase=0;phase<8;++phase) {
            const auto reading=detector::scan(level,live,camera,phase,true);
            if (!reading.count) continue;
            current=reading.contacts[0];
            check(current->x==x && current->y==y,
                  "Radar differs from the rendered position of a moving enemy");
            checkPixels(reading);
        }
        if (previous && current) moved|=previous->x!=x || previous->y!=y;
        if (previousSector==11 && record.sector==10) {
            check(previous && current && previous->x==x && previous->y==y,
                  "Radar marker jumped when the real enemy changed sector origin");
            crossed=true;
        }
        previous=current;
        previousSector=record.sector;
    }
    check(moved && crossed,"Original movement and sector crossing were not both exercised");
    std::cout<<"Original level 2 cardinal, live pursuit, sector crossing and CASQUE pixels passed\n";
}
}
int main(int argc,char** argv)
{
    try {
        if (argc>=2) original(argv[1]);
        if (argc>=3) originalLevel(argv[2]);
        scanRules();
        battery();
        std::cout<<"Original motion detector and battery passed\n";
    }
    catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
