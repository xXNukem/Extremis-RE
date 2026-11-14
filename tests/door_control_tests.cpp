#include "door_control.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
using namespace std::chrono_literals;
constexpr double pi=3.14159265358979323846;
void check(bool value,const char* message) { if (!value) throw std::runtime_error(message); }
game_lab::Level fixture()
{
    game_lab::Level level;
    game_lab::Sector a{},b{};
    a.vertices={{500,300},{500,100},{0,100},{0,300}};
    a.walls={{0,1,0x1017,0}}; a.neighbors={1}; a.polygons={{{0,1,2,3},0,{}}};
    b.gx=500; b.vertices={{0,100},{0,300},{500,300},{500,100}};
    b.walls={{0,1,0x1021,0}}; b.neighbors={0}; b.polygons={{{0,1,2,3},0,{}}};
    level.sectors={a,b}; level.connectionStates={0};
    return level;
}
Camera camera(double x,double y,double angle,unsigned sector)
{
    Camera c{x,y,angle}; c.sector=sector; return c;
}
struct World {
    game_lab::Level level=fixture();
    std::vector<doors::Door> doors=doors::closedDoors(level);
    collision::World walls=collision::staticWorld(level);
    DoorControl control{walls,doors,level,DoorControl::Time{}};
};
void requests()
{
    for (const bool reverse:{false,true}) {
        World w;
        const auto before=camera(reverse?680:320,200,reverse?pi:0,reverse?1:0);
        auto next=before; next.x+=reverse?-10:10;
        w.control.constrainPlayer(before,next);
        check(w.level.connectionStates[0]==0x100,"Both faces must request the same word");
        check(next.x==before.x && next.sector==before.sector,"Opening must keep normal 180 and sector");
        check(doors::connectionEnabled(w.level,0),"Request must connect at phase zero");
        w.level.connectionStates[0]=59;
        w.control.constrainPlayer(before,next);
        check(w.level.connectionStates[0]==0x13b,"Descending request must reopen, not toggle");
    }
    for (int heading:{119,120,239,240}) {
        World w;
        auto before=camera(320,200,(540+heading)*pi/360,0),next=before;
        w.control.constrainPlayer(before,next);
        check(bool(w.level.connectionStates[0]&0x100)==(heading>=120 && heading<240),"Angular boundary");
    }
    for (int normal:{180,181}) {
        World w; auto before=camera(500-normal,200,0,0),next=before;
        w.control.constrainPlayer(before,next);
        check(bool(w.level.connectionStates[0]&0x100)==(normal==180),"Normal proximity boundary");
    }
    for (int tangent:{-1,0,200,201}) {
        World w; auto before=camera(320,300-tangent,0,0),next=before;
        w.control.constrainPlayer(before,next);
        check(bool(w.level.connectionStates[0]&0x100)==(tangent>=0 && tangent<=200),"Tangential proximity boundary");
    }
    World w;
    auto before=camera(310,250,0,0),next=before; next.x=330;
    w.control.constrainPlayer(before,next);
    check(w.level.connectionStates[0]==0x100 && next.x==320 && next.y==240,
          "Swept proximity / one tangential step of 10");
    // No call for enemy substeps: the aggregate proposal alone owns the correction.
    World split;
    auto aggregate=before;
    for (int i=0;i<4;++i) aggregate.x+=5;
    split.control.constrainPlayer(before,aggregate);
    check(aggregate.x==next.x && aggregate.y==next.y,"Substeps multiplied correction");
    w.level.connectionStates[0]=0;
    w.level.sectors[0].walls[0].type|=0x0800;
    next=before; next.x=330;
    w.control.constrainPlayer(before,next);
    check(w.level.connectionStates[0]==0,"Special wall accepted ordinary request");
    check(next.x==318,"Closed connection clearance correction");
}
void diagonal()
{
    World w;
    w.level.sectors[0].vertices={{500,500},{641,359}};
    w.level.sectors[1].gx=0;
    w.level.sectors[1].vertices={{641,359},{500,500}};
    w.doors=doors::closedDoors(w.level);
    auto before=camera(443,302,pi/4,0),next=before;
    next.x=450; next.y=310;
    w.control.constrainPlayer(before,next);
    check(w.level.connectionStates[0]==0x100 && next.x==445 && next.y==301,
          "Diagonal Q14 correction / negative floor rounding");
}
void crossing()
{
    for (const bool reverse:{false,true}) {
        for (const int phase:{59,60,61,64,89,90}) {
            World w; w.level.connectionStates[0]=0x100|phase;
            const auto before=camera(reverse?520:480,200,reverse?pi:0,reverse?1:0);
            auto next=before; // Remain on the near side first.
            w.control.constrainPlayer(before,next);
            check(next.sector==before.sector,"Approach changed sector early");
            check(w.level.connectionStates[0]==(phase>60 ? 0x140 : (0x100|phase)),"Phase 64 entry threshold");
            if (phase<=60) { check(next.x==(reverse?680:320),"Invisible phase 60 allowed passage"); continue; }
            next=before; next.x=reverse?490:510;
            w.control.constrainPlayer(before,next);
            check(next.sector==unsigned(reverse?0:1) && next.x==(reverse?490:510),
                  "Crossing lost authoritative sector or world coordinate");
            check(w.level.connectionStates[0]==0x140,"Crossing did not write phase 64");
        }
    }
    World w; w.level.connectionStates[0]=0x541;
    auto before=camera(480,250,pi,0),next=before; next.x=510;
    w.control.constrainPlayer(before,next);
    check(next.y==218 && next.sector==1 && w.level.connectionStates[0]==0x540,
          "Crossing clamp/lock preservation must not require a fresh open request");
    w.level.connectionStates[0]=64; // Descending, facing away: visually hidden but no passage.
    next=before;
    w.control.constrainPlayer(before,next);
    check(next.x==320 && next.sector==0 && w.level.connectionStates[0]==64,"Descending passage gate");
    // Unknown/unpaired connections must never receive automatic door requests.
    w.doors.clear(); w.level.connectionStates[0]=0;
    before=camera(480,200,0,0); next=before; next.x=510;
    w.control.constrainPlayer(before,next);
    check(next.sector==1 && w.level.connectionStates[0]==0,"Open portal sector tracking wrote door state");
}
void access()
{
    World w; w.level.connectionStates[0]=0x400;
    auto before=camera(310,200,0,0),next=before; next.x=330;
    check(w.control.constrainPlayer(before,next),"Locked approach not reported");
    check(w.level.connectionStates[0]==0x400 && next.x==318,"Locked door opened or passed");
    using R=DoorControl::CardResult;
    check(w.control.useCardReader(0x0100,{})==R::NoCard,"No card");
    check(w.control.useCardReader(0x0100,1)==R::Rejected,"Wrong card family");
    w.level.connectionStates[0]=0x53b;
    check(w.control.useCardReader(0x0100,0x1001)==R::Unlocked && w.level.connectionStates[0]==0x13b,
          "Card must change only lock");
    check(w.control.useCardReader(0x0100,0x1001)==R::AlreadyDisabled,"Repeated card re-locked");
    check(w.control.useCardReader(0x0101,0x1001)==R::InvalidTarget,"Missing connection accepted card");
}
void timing()
{
    World w; const DoorControl::Time zero{};
    check(!w.control.beginFrame(true,zero),"Entry interval must be discarded");
    check(w.control.beginFrame(true,zero+7ms),"Active interval rejected");
    auto before=camera(320,200,0,0),next=before;
    w.control.constrainPlayer(before,next);
    check(w.level.connectionStates[0]==0x100,"Movement must precede ticking");
    w.control.updateDoors(zero+7ms);
    check(w.level.connectionStates[0]==0x101,"First tick after request");
    w.control.resetCounter(zero+7ms);
    w.control.beginFrame(false,zero+10ms);
    w.control.updateDoors(zero+10s);
    check(w.level.connectionStates[0]==0x101,"Paused door advanced");
    check(!w.control.beginFrame(true,zero+20s),"Resume interval was replayed");
    w.control.resetCounter(zero+20s);
    check(w.control.beginFrame(true,zero+20s+7ms),"Resume failed");
    w.control.updateDoors(zero+20s+7ms);
    check(w.level.connectionStates[0]==0x102,"Resume lost phase or caught up inactive time");
    // Full autonomous cycle through the controller; no more movement requests.
    for (int i=0;i<2000 && w.level.connectionStates[0];++i) {
        const auto t=zero+20s+7ms+std::chrono::milliseconds(7*(i+1));
        w.control.resetCounter(t-7ms); w.control.beginFrame(true,t); w.control.updateDoors(t);
    }
    check(w.level.connectionStates[0]==0,"Door failed to close automatically");
    w.control.resetLevel(zero+60s);
    check(!w.control.beginFrame(true,zero+61s),"Level travel replayed loading time");
}
void originalSecurity(const std::filesystem::path& path)
{
    auto level=game_lab::loadLevel1(path).level;
    const auto doors=doors::closedDoors(level);
    const auto words=level.connectionStates;
    const auto walls=collision::staticWorld(level);
    DoorControl control(walls,doors,level);
    check(doors.size()==15,"Original level-1 door recognition changed");
    unsigned approaches=0;
    for (const auto& door:doors) for (const auto& face:door.faces) {
        for (unsigned sectorIndex=0;sectorIndex<level.sectors.size();++sectorIndex) {
            const auto& sector=level.sectors[sectorIndex];
            for (unsigned i=0;i<sector.neighbors.size() && i<sector.walls.size();++i) {
                const auto& wall=sector.walls[i];
                const auto a=sector.vertices[wall.vertexA],b=sector.vertices[wall.vertexB];
                if (wall.extra!=door.stateIndex || sector.gx+a.x!=face.a.x || sector.gy+a.y!=face.a.y ||
                    sector.gx+b.x!=face.b.x || sector.gy+b.y!=face.b.y) continue;
                const double dx=face.b.x-face.a.x,dy=face.b.y-face.a.y;
                const double length=std::hypot(dx,dy),nx=dy/length,ny=-dx/length;
                const double mx=(face.a.x+face.b.x)/2,my=(face.a.y+face.b.y)/2;
                auto before=camera(mx+nx*180,my+ny*180,std::atan2(-ny,-nx),sectorIndex);
                auto next=before; next.x-=nx*10; next.y-=ny*10;
                level.connectionStates=words;
                control.constrainPlayer(before,next);
                const bool accepts=!(words[door.stateIndex]&0x400) && !(wall.type&0x0800);
                check(bool(level.connectionStates[door.stateIndex]&0x100)==accepts,
                      "Original face approach request/access mismatch");
                level.connectionStates[door.stateIndex]=0x13d;
                before.x=mx+nx*20; before.y=my+ny*20;
                next=before; next.x=mx-nx*10; next.y=my-ny*10;
                control.constrainPlayer(before,next);
                check(next.sector==sector.neighbors[i] && level.connectionStates[door.stateIndex]==0x140,
                      "Original face failed phase-64 sector crossing");
                ++approaches;
            }
        }
    }
    check(approaches==30,"Did not exercise both faces of all fifteen doors");
    level.connectionStates=words;

    check(control.useCardReader(0x0107,0x1001)==DoorControl::CardResult::Unlocked,"Original C001 reader");
    for (unsigned i=0;i<words.size();++i)
        check(level.connectionStates[i]==(i==7 ? (words[i]&~0x400) : words[i]),"Card changed another connection");
}
}
int main(int argc,char** argv)
{
    try {
        requests(); diagonal(); crossing(); access(); timing();
        if (argc==2) originalSecurity(argv[1]);
        std::cout<<"DOS connection movement, sectors, access and timing passed.\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
