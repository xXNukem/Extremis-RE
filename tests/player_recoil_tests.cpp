#include "enemy_system.h"
#include "player_control.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
bool near(double a,double b) { return std::abs(a-b)<1e-7; }
void write(const std::filesystem::path& path,const std::vector<unsigned char>& bytes)
{
    std::ofstream out(path,std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
    check(bool(out),"Cannot write recoil fixture");
}
void fixture(const std::filesystem::path& directory)
{
    std::filesystem::create_directories(directory);
    std::vector<unsigned char> bytes(84,0);bytes[2]=20;
    for(unsigned i=0;i<20;++i) {
        unsigned offset=bytes.size()-4;
        for(unsigned j=0;j<4;++j) bytes[4+i*4+j]=(offset>>(8*j))&255;
        bytes.insert(bytes.end(),{2,0,2,0,1,0,2,0,0,1,1,1});
    }
    write(directory/"MONS1.SPR",bytes);
    write(directory/"PAL1.PAL",std::vector<unsigned char>(768,63));
    write(directory/"MONS1.SPT",std::vector<unsigned char>(160,0));
}
struct Session {
    game_lab::Level level{};
    enemies::System enemies;
    player::Vitals vitals;
    Camera camera{500,500,0};
    collision::World walls;
    unsigned pushes=0;
    Session(const std::filesystem::path& directory,bool restart=false,bool blocker=false) {
        level.monsBank=level.palBank=1;level.sectors.resize(1);
        auto& sector=level.sectors[0];
        sector.vertices={{0,0},{2000,0},{2000,2000},{0,2000}};
        sector.polygons={{{0,1,2,3},0,{}}};
        // Already attacking: phase 2 triggers one shared reaction next tick.
        level.enemies={{4,0,2,1,0,700,500,0,0,0,0x8000,0,700,500}};
        if(restart) {auto e=level.enemies[0];e.phase=0;e.y=800;level.enemies.push_back(e);}
        if(blocker) {auto e=level.enemies[0];e.state=6;e.x=300;level.enemies.push_back(e);}
        enemies.load(level,directory);enemies.update(level,camera,walls,vitals,0,true);
    }
    void frame(double dt,player::Input input={},bool active=true) {
        enemies.update(level,camera,walls,vitals,dt,active,[&](double seconds){
            const auto before=camera;input.blockMovement=enemies.reacting();
            player::update(camera,input,seconds,{},&walls);input.mouseX=0;
            enemies.constrainPlayer(level,before,camera,walls);
        },[&]{
            const auto before=camera;player::recoil(camera,walls);
            enemies.constrainPlayer(level,before,camera,walls);++pushes;
        });
    }
    void run(double duration,int fps,player::Input input={}) {
        while(duration>1e-12) {const double dt=std::min(duration,1./fps);frame(dt,input);duration-=dt;}
    }
};
void cases(const std::filesystem::path& directory)
{
    for(int fps:{30,60,120}) {
        Session plain(directory);plain.run(8./18,fps);
        check(plain.pushes==8 && near(plain.camera.x,340) && near(plain.camera.y,500) &&
              plain.vitals.health()==984 && !plain.enemies.reacting(),"Eight ticks / damage / FPS differ");
        Session blocked(directory);blocked.walls={{{400,0},{400,2000}}};blocked.run(8./18,fps);
        check(near(blocked.camera.x,430.0001) && blocked.pushes==8 && blocked.vitals.health()==984,
              "Wall recoil collision or damage differs");
        Session body(directory,false,true);body.run(8./18,fps);
        check(body.camera.x>=400 && body.camera.x<401,"Recoil crossed enemy collision box");
        Session held(directory);held.frame(1./18);
        held.run(7./18,fps,{1,1,0,0,true,true});
        check(near(held.camera.x,340) && near(held.camera.y,500) && held.camera.postureHeight==100,
              "Voluntary movement not suppressed or crouch stopped");
        held.frame(1./18,{1,0});check(held.camera.x>340,"Movement stayed blocked after reaction");
        Session turning(directory);turning.run(8./18,fps,{0,0,1});
        double x=500,y=500;
        for(int i=1;i<=8;++i){x-=20*std::cos(i*1.5/18);y-=20*std::sin(i*1.5/18);}
        check(near(turning.camera.x,x) && near(turning.camera.y,y) && near(turning.camera.angle,8*1.5/18),
              "Recoil did not follow current turning angle at fixed ticks");
        Session restarted(directory,true);restarted.run(9./18,fps);
        check(restarted.pushes==9 && near(restarted.camera.x,320) && restarted.vitals.health()==982 &&
              !restarted.enemies.reacting(),"New attack failed to restart shared reaction");
        Session paused(directory);paused.frame(1./18);paused.frame(.2,{},false);
        check(paused.pushes==1 && paused.vitals.health()==998,"Inactive reaction advanced");
        paused.frame(.2);check(paused.pushes==1,"Resume applied paused time");
        paused.run(7./18,fps);check(paused.pushes==8,"Resume lost pending reaction");
    }
}
}
int main(int argc,char** argv)
{
    try {check(argc==2,"Expected fixture directory");fixture(argv[1]);cases(argv[1]);
        std::cout<<"Fixed-step recoil, walls/enemies, turn, crouch, restart, pause and 30/60/120 FPS passed\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
