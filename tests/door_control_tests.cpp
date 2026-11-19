#include "extremis/world/door_control.h"
#include "extremis/gameplay/player/player_control.h"
#include "extremis/gameplay/player/weapon_fire.h"
#include <algorithm>
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
void blockedCrossing()
{
    for (bool reverse:{false,true}) for (int state:{0,0x400,0x100,0x13b,0x13c,59,64,90}) {
        for (double lateral:{0.,12.}) for (int distance:{40,350}) {
            World w;
            w.level.connectionStates[0]=state;
            const auto before=camera(reverse?520:480,200,reverse?0:pi,reverse?1:0);
            auto next=before;
            next.x+=reverse?-distance:distance;
            next.y+=lateral;
            w.control.constrainPlayer(before,next);
            check((reverse?next.x>500:next.x<500) && next.sector==before.sector,
                  "Unpassable connection let a backing or diagonal move cross its plane");
            check(w.level.connectionStates[0]==state,"Backing through a door requested opening");
        }
    }
    World w;
    w.level.sectors[0].vertices={{500,500},{640,379}};
    w.level.sectors[1].gx=0;
    w.level.sectors[1].vertices={{640,379},{500,500}};
    w.doors=doors::closedDoors(w.level);
    // The proposed point is 0.001 beyond the diagonal plane, but the
    // rounded Q14 normal is positive on this face.
    const auto before=camera(556.8114888981,449.7090780640,-2.2835257412,0);
    auto next=camera(557.4006539012,450.3907565799,before.angle,0);
    w.control.constrainPlayer(before,next);
    check(next.x==before.x && next.y==before.y,"Rounded Q14 normal hid a closed diagonal crossing");
}
void projectileDoors()
{
    World w;
    for (unsigned phase:{0,15,30,59,60,61,90}) for (bool opening:{false,true}) {
        w.level.connectionStates[0]=phase|(opening?0x100:0);
        w.control.rebuildWorld();
        for (bool reverse:{false,true}) for (double y:{125.,175.,225.,275.}) {
            const collision::Point start{reverse?550.:450.,y};
            const collision::Point direction{reverse?-1.:1.,0};
            const auto& door=w.doors.front();
            const auto face=doors::visibleFace(w.level,door,door.faces[reverse?1:0]);
            const auto expected=face && y>=std::min(face->a.y,face->b.y) &&
                y<=std::max(face->a.y,face->b.y);
            const auto actual=collision::rayDistance(w.control.projectileLeaves(),start,direction,100,start);
            check(expected ? actual==50 : !actual,"Projectile hit disagrees with the facing door leaf");
        }
    }
}
void chainedCrossing()
{
    World w;
    auto& middle=w.level.sectors[1];
    middle.vertices.insert(middle.vertices.end(),{{150,300},{150,100}});
    middle.walls.push_back({4,5,0x1017,1});
    middle.neighbors.push_back(2);
    auto last=w.level.sectors[1];
    last.gx=650;
    last.walls.resize(1); last.walls[0].extra=1;
    last.neighbors={1};
    w.level.sectors.push_back(last);
    w.level.connectionStates.push_back(0);
    w.doors=doors::closedDoors(w.level);
    w.level.connectionStates[0]=0x141;
    const auto before=camera(480,200,pi,0);
    auto next=before; next.x=660; next.y=205;
    w.control.constrainPlayer(before,next);
    check(next.x==480 && next.y==200 && next.sector==0 && w.level.connectionStates[0]==0x141,
          "One aggregate move crossed an open connection and then a closed connection");
}
void projectileUpdates()
{
    for (bool closes:{false,true}) {
        World w;
        const auto shooter=camera(390,225,0,0);
        w.walls.push_back({{560,100},{560,300}});
        w.level.connectionStates[0]=0x100|30;
        w.control.rebuildWorld();
        player::WeaponFire fire;
        player::Inventory inventory;
        std::uint16_t contents=10;
        check(inventory.exchangeWeapon(contents)==player::Transfer::Exchanged,"Test weapon equip");
        check(fire.update(shooter,w.control.staticWorld(),inventory,{true,true},true,0,
                          nullptr,w.control.projectileLeaves())==player::FireResult::Fired,
              "Projectile did not launch in the doorway test");
        check(!fire.scene().objects.at(0).wallImpact,"Door hit beyond one projectile step");
        w.level.connectionStates[0]=closes?15:30;
        w.control.rebuildWorld();
        fire.update(shooter,w.control.staticWorld(),inventory,{},true,1./60,
                    nullptr,w.control.projectileLeaves());
        const auto& object=fire.scene().objects.at(0);
        check(object.wallImpact==closes && (closes?std::abs(object.x-500)<.001:object.x==550),
              "In-flight shot ignored the current leaf pose");
        if (!closes) {
            fire.update(shooter,w.control.staticWorld(),inventory,{},true,1./60,
                        nullptr,w.control.projectileLeaves());
            check(fire.scene().objects.at(0).wallImpact && std::abs(fire.scene().objects.at(0).x-560)<.001,
                  "Through-shot did not hit the next static surface");
        }
    }
}
void enemyClosingAgainstStaticWall()
{
    World w;
    w.level.connectionStates[0]=59;
    w.walls.push_back({{400,100},{400,300}});
    w.control.rebuildWorld();
    const auto moved=doors::moveEnemy(w.level,w.doors,w.control.movementWorld(),
                                      0,{470,200},{20,0},true);
    check(moved.x>430 && moved.x<431 && moved.y==200,
          "Closing door clearance moved an actor through a static wall");
    w.level.connectionStates[0]=0;
    w.walls.clear();
    w.control.rebuildWorld();
    const auto blocked=doors::moveEnemy(w.level,w.doors,w.control.movementWorld(),
                                        0,{410,200},{40,0},false);
    check(blocked.x==418 && blocked.y==200,"Generic enemy mode lost the DOS 80+2 clearance");
}
void originalDoorCampaign(const std::filesystem::path& path)
{
    unsigned faces=0, locked=0, diagonal=0;
    for (unsigned zone=0;zone<30;++zone) {
        auto level=game_lab::loadLevel(path,zone).level;
        const auto initial=level.connectionStates;
        const auto doors=doors::closedDoors(level);
        const auto walls=collision::staticWorld(level);
        DoorControl control(walls,doors,level);
        for (unsigned sectorIndex=0;sectorIndex<level.sectors.size();++sectorIndex) {
            const auto& sector=level.sectors[sectorIndex];
            for (unsigned i=0;i<sector.neighbors.size() && i<sector.walls.size();++i) {
                const auto& wall=sector.walls[i];
                if ((wall.type&0xf000)!=0x1000) continue;
                const auto door=std::find_if(doors.begin(),doors.end(),[&](const auto& d) {
                    return d.stateIndex==wall.extra;
                });
                if (door==doors.end()) continue;
                const auto a=sector.vertices[wall.vertexA], b=sector.vertices[wall.vertexB];
                const double dx=double(b.x)-a.x, dy=double(b.y)-a.y, length=std::hypot(dx,dy);
                const double nx=dy/length, ny=-dx/length;
                const double mx=sector.gx+(a.x+b.x)/2., my=sector.gy+(a.y+b.y)/2.;
                level.connectionStates=initial;
                const auto before=camera(mx+20*nx,my+20*ny,std::atan2(ny,nx),sectorIndex);
                auto next=before; next.x-=40*nx; next.y-=40*ny;
                control.constrainPlayer(before,next);
                check(next.x==before.x && next.y==before.y && next.sector==before.sector,
                      "Closed original face failed the swept backing check");
                if (dx==0 || dy==0) {
                    const collision::Point start{mx+190*nx,my+190*ny};
                    const auto enemy=doors::moveEnemy(level,doors,{},sectorIndex,start,
                                                      {-30*nx,-30*ny},true);
                    check((enemy.x-mx)*nx+(enemy.y-my)*ny==182,
                          "Enemy proposal ignored an original closed door clearance");
                }
                level.connectionStates[wall.extra]=(initial[wall.extra]&0x400)|30;
                control.rebuildWorld();
                for (double fraction:{0.25,0.75}) {
                    const collision::Point start{sector.gx+a.x+dx*fraction+40*nx,
                                                 sector.gy+a.y+dy*fraction+40*ny};
                    const auto hit=collision::rayDistance(control.projectileLeaves(),
                                                          start,{-nx,-ny},45,start);
                    check(fraction<.5 ? !hit : hit && std::abs(*hit-40)<1e-8,
                          "Original moving leaf did not match the expected ray aperture");
                }
                ++faces;
                locked+=bool(initial[wall.extra]&0x400);
                diagonal+=dx!=0 && dy!=0;
            }
        }
    }
    std::cout<<"30 levels, "<<faces<<" directed door faces ("<<locked<<" locked, "
             <<diagonal<<" diagonal).\n";
    check(faces>30 && locked>0,"Campaign door coverage incomplete");
}
void movementApproach()
{
    // Exercise player/static walls then one connection correction, before the
    // phase update. The request boundary is also the stopping line.
    for (bool reverse:{false,true}) for (bool run:{false,true})
        for (double dt:{1.0/60,1.0/18}) {
            World w;
            auto c=camera(reverse?720:280,200,reverse?pi:0,reverse?1:0);
            player::Input input; input.forward=1; input.run=run;
            bool requested=false;
            for (int frame=0;frame<30 && !requested;++frame) {
                const auto before=c;
                player::update(c,input,dt,{},&w.control.staticWorld());
                const int normal=int(std::lround(reverse?c.x-500:500-c.x));
                w.control.constrainPlayer(before,c);
                requested=bool(w.level.connectionStates[0]&0x100);
                check(requested==(normal<=180),"Movement/static collision delayed request boundary");
            }
            check(requested && c.x==(reverse?680:320),"Approach did not stop at request line");
            for (int phase:{0,59,60,61}) {
                w.level.connectionStates[0]=0x100|phase;
                c=camera(reverse?680:320,200,reverse?pi:0,reverse?1:0);
                const auto before=c;
                player::update(c,input,dt,{},&w.control.staticWorld());
                w.control.constrainPlayer(before,c);
                check((c.x!=before.x)==(phase>60),"Movement must wait through phase 60");
            }
        }
}
void movementDirections()
{
    struct Direction { double heading, forward, right; bool facing; };
    const Direction directions[]={{0,1,0,true},{pi,-1,0,false},{pi/2,0,1,false},
                                  {pi/4,1,1,false}};
    for (bool reverse:{false,true}) for (bool run:{false,true})
        for (double dt:{.004,.033,.25}) for (auto direction:directions)
            for (int state:{0,0x400,0x13b,0x13c,0x13d,59,60,90}) {
                World w;
                w.level.connectionStates[0]=state;
                auto c=camera(reverse?520:480,200,direction.heading+(reverse?pi:0),reverse?1:0);
                player::Input input;
                input.forward=direction.forward;
                input.right=direction.right;
                input.run=run;
                const bool passable=state==0x13d || (state==90 && direction.facing);
                for (unsigned frame=0;frame<unsigned(.3/dt)+1 && c.sector==unsigned(reverse);++frame) {
                    const auto before=c;
                    player::update(c,input,dt,{},&w.control.staticWorld());
                    w.control.constrainPlayer(before,c);
                }
                check(passable ? c.sector==unsigned(!reverse)
                               : c.sector==unsigned(reverse) && (reverse?c.x>500:c.x<500),
                      "Walking/running with forward, back, strafe or diagonal input misread the passage gate");
                if (!state) check(w.level.connectionStates[0]==(direction.facing?0x100:0),
                                  "Movement direction substituted for facing in the opening request");
            }
}
void fractionalPassage()
{
    for (bool reverse:{false,true}) for (bool run:{false,true})
        for (double angle:{0.0,0.04}) for (double dt:{0.001,0.004,0.007,0.016,0.033,0.056}) {
            World w;
            auto c=camera(reverse?690:310,200,(reverse?pi:0)+angle,reverse?1:0);
            player::Input input;
            input.forward=1;
            input.run=run;
            w.level.connectionStates[0]=0x13d;
            w.control.rebuildWorld();
            check(w.control.movementWorld().size()==w.control.staticWorld().size(),
                  "Traversable door still had a blocking segment");
            bool crossed=false;
            for (int frame=0;frame<1500 && std::abs(c.x-500)<200;++frame) {
                const auto before=c;
                player::update(c,input,dt,{},&w.control.staticWorld());
                const auto requested=c;
                w.control.constrainPlayer(before,c);
                check(std::abs(c.x-requested.x)<1e-9 && std::abs(c.y-requested.y)<1e-9,
                      "Open passage discarded fractional player movement");
                crossed=crossed || c.sector!=before.sector;
            }
            check(std::abs(c.x-500)>=200 && crossed && c.sector==unsigned(reverse?0:1) &&
                  w.level.connectionStates[0]==0x140,
                  "Fractional crossing lost the sector transition or phase-64 write");
        }
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
        projectileDoors(); projectileUpdates(); requests(); diagonal(); blockedCrossing();
        chainedCrossing(); enemyClosingAgainstStaticWall(); movementApproach(); movementDirections();
        fractionalPassage(); crossing(); access(); timing();
        if (argc==2) { originalSecurity(argv[1]); originalDoorCampaign(argv[1]); }
        std::cout<<"DOS connection movement, sectors, access and timing passed.\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
