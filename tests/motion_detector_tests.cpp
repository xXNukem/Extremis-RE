#include "motion_detector.h"
#include "player_inventory.h"
#include "game_lab.h"
#include "../src/motion_detector_tables.h"
#include <fstream>
#include <iostream>
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
    reading=detector::scan(level,{enemy(640,0)},{3000,3000,1.5707963267948966},1,true);
    check(reading.count==1 && reading.contacts[0].x==14 && reading.contacts[0].y==198,"Native X sign was silently inverted");
    check(detector::scan(level,{enemy(2048,0)},camera,4,true).count==0,"Forward32 bound included");
    check(detector::scan(level,{enemy(2047,0)},camera,4,true).count==1,"Forward31 bound excluded");
    check(detector::scan(level,{enemy(640,1728)},camera,7,true).count==0,"Lateral27 bound included");
    auto stationary=enemy(640,0);
    check(detector::scan(level,{stationary},camera,4,true).count==1,"Stationary enemy was excluded");
    stationary.health=0; stationary.state=5;
    check(detector::scan(level,{stationary},camera,4,true).count==1,"Dying enemy excluded before native removal");
    stationary.health=0xffff;
    check(detector::scan(level,{stationary},camera,4,true).count==0,"Removed enemy still detected");
    check(!detector::scan(level,{enemy(640,0)},camera,4,false).phase,"Off detector animated");
    std::vector<game_lab::EnemyRecord> many(11,enemy(640,0));
    check(detector::scan(level,many,camera,4,true).count==10,"Native ten-contact cap changed");
    // No geometry/door or velocity dependency exists in the original scan.
    level.sectors[0].walls={{0,1,1,0}};
    check(detector::scan(level,{enemy(640,0)},camera,4,true).count==1,"Wall topology affected detector");
}
void battery()
{
    player::Inventory inv;
    check(inv.batteryCharge()==-1 && !inv.toggleDetector(),"Native initial absent battery changed");
    std::uint16_t item=1;
    check(inv.takeSupply(item)==player::Transfer::Taken && item==0xffff && inv.batteryCharge()==160,
          "Original seconds-to-PIT battery conversion failed");
    check(inv.toggleDetector(),"Charged detector did not activate");
    inv.updateBattery(10,false); check(inv.batteryCharge()==160,"Paused battery drained");
    for(unsigned i=0;i<30;++i) inv.updateBattery(1./60,true);
    check(inv.batteryCharge()==80,"Battery clock depends on render chunks");
    inv.updateBattery(.5,true);
    check(inv.batteryCharge()==0 && !inv.detectorEnabled(),"Exhausted detector did not turn off");
    player::Inventory slow,fast; std::uint16_t a=900,b=900;
    slow.takeSupply(a);fast.takeSupply(b);slow.toggleDetector();fast.toggleDetector();
    for(unsigned i=0;i<30;++i) slow.updateBattery(1./30,true);
    for(unsigned i=0;i<120;++i) fast.updateBattery(1./120,true);
    check(slow.batteryCharge()==fast.batteryCharge(),"Battery drain differs at30/120 FPS");
}
void original(const char* path)
{
    std::ifstream file(path,std::ios::binary);
    const std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(file),{}};
    check(bytes.size()>0x3da5,"Cannot read native EXE");
    for(unsigned i=0;i<720;++i) {
        const auto word=std::int16_t(unsigned(bytes[0x3164+2*i])|(unsigned(bytes[0x3165+2*i])<<8));
        check(detector::cosine[i]==word,"Radar Q14 cosine differs from EXE");
    }
    for(unsigned i=0;i<257;++i) check(detector::arctangent[i]==bytes[0x3ca4+i],"Radar atan table differs from EXE");
}
}
int main(int argc,char** argv)
{
    try {scanRules();battery();if(argc==2) original(argv[1]);std::cout<<"Original motion detector and battery passed\n";}
    catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
