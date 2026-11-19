#include "extremis/gameplay/player/player_control.h"
#include "extremis/world/objet_collision.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
using collision::Point;
void check(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}
bool near(double a, double b) { return std::abs(a - b) < 0.001; }
const collision::Settings body{10, 0.0001, 8};

void testWallAndSliding()
{
    const collision::World world{{{100, -1000}, {100, 1000}}};
    const auto hit = collision::move(world, {0, 0}, {200, 0}, body);
    check(near(hit.x, 90) && near(hit.y, 0), "Circle passed through wall");
    const auto slide = collision::move(world, {0, 0}, {200, 100}, body);
    check(near(slide.x, 90) && near(slide.y, 100), "Tangential displacement was lost");
    const auto parallel = collision::move(world, {90, 0}, {0, 50}, body);
    check(near(parallel.x, 90) && near(parallel.y, 50), "Touching circle cannot slide");
    const auto inward = collision::move(world, {90, 0}, {100, 0}, body);
    const auto away = collision::move(world, {90, 0}, {-20, 0}, body);
    check(near(inward.x, 90) && near(away.x, 70), "Contact blocks escape or admits penetration");
    const auto reverse = collision::move(world, {200, 0}, {-200, 0}, body);
    check(near(reverse.x, 110), "Wall is not solid from both sides");
}

void testCorners()
{
    collision::World corner{{{100, -100}, {100, 100}}, {{-100, 100}, {100, 100}}};
    auto result = collision::move(corner, {0, 0}, {300, 300}, body);
    check(near(result.x, 90) && near(result.y, 90), "Circle crossed solid corner");
    std::reverse(corner.begin(), corner.end());
    result = collision::move(corner, {0, 0}, {300, 300}, body);
    check(near(result.x, 90) && near(result.y, 90), "Corner depends on segment order");
    const collision::World narrow{{{100, -100}, {100, -5}}, {{100, 5}, {100, 100}}};
    result = collision::move(narrow, {0, 0}, {10000, 0}, body);
    check(result.x < 100, "Circle squeezed through gap smaller than diameter");
}

void testObliqueAndEndpoints()
{
    const collision::World diagonal{{{-1000, -900}, {1000, 1100}}};
    const auto hit = collision::move(diagonal, {0, 0}, {0, 200}, body);
    const double separation = (hit.y - hit.x - 100) / std::sqrt(2.0);
    check(near(separation, -10) && hit.x > 0, "Oblique collision/slide failed");
    const collision::World wall{{{100, 0}, {100, 100}}};
    const auto end = collision::move(wall, {0, 100}, {200, 0}, body);
    check(near(end.x, 90), "Segment endpoint not covered");
    const auto cap = collision::move(wall, {0, 105}, {200, 0}, body);
    check(cap.x < 200 && cap.y > 105 && std::hypot(cap.x - 100, cap.y - 100) >= 10,
          "Rounded endpoint sweep failed");
    const auto grazing = collision::move(wall, {0, 110}, {200, 0}, body);
    check(near(grazing.x, 200) && near(grazing.y, 110), "Tangent endpoint catches movement");
    const collision::World point{{{100, 0}, {100, 0}}};
    check(near(collision::move(point, {0, 0}, {200, 0}, body).x, 90),
          "Degenerate segment does not behave as an endpoint");
}

void testFastMotionAndTime()
{
    const collision::World wall{{{100, -10000}, {100, 10000}}};
    for (double speed : {200.0, 10000.0, 10000000.0}) {
        const auto hit = collision::move(wall, {0, 0}, {speed, 0}, body);
        check(near(hit.x, 90), "Fast movement tunneled through wall");
    }
    auto fine = Point{0, 0};
    for (int i = 0; i < 60; ++i) fine = collision::move(wall, fine, {200.0 / 60, 100.0 / 60}, body);
    const auto coarse = collision::move(wall, {0, 0}, {200, 100}, body);
    check(near(fine.x, coarse.x) && near(fine.y, coarse.y), "Sliding changes with time subdivision");
    const collision::Settings limited{10, 0.0001, 1};
    check(collision::move(wall, {0, 0}, {200, 100}, limited).x <= 90,
          "Contact iteration limit applied unchecked motion");
}

game_lab::Level portalFixture()
{
    game_lab::Level level{};
    game_lab::Sector sector{};
    sector.vertices = {{100, 0}, {100, 40}, {100, 60}, {100, 100}};
    sector.walls = {{0, 1, 0x2003, 0}, {1, 2, 0x1035, 0}, {2, 3, 0x3002, 0}};
    // Internal polygon edges must not become new blocking walls.
    sector.polygons = {{{0, 1, 2, 3}, 0, {}}};
    level.sectors.push_back(sector);
    level.connectionStates = {0}; // Closed door still excluded from this phase.
    return level;
}

void testPortalsAndGeometry()
{
    auto level = portalFixture();
    const auto world = collision::staticWorld(level);
    check(world.size() == 2 && level.sectors[0].walls.size() == 3, "Geometry changed or portal became solid");
    const collision::Settings small{5, 0.0001, 8};
    const auto crossing = collision::move(world, {0, 50}, {200, 0}, small);
    check(near(crossing.x, 200), "Free portal or closed dynamic door blocks static movement");
    check(near(collision::move(world, {0, 20}, {200, 0}, small).x, 95), "Frame became passable");
    level.sectors[0].gx = 65000;
    level.sectors[0].gy = 64000;
    const auto shifted = collision::staticWorld(level);
    check(shifted[0].a.x == 65100 && shifted[1].b.y == 64100, "Global wall coordinates overflowed");
}

void testPlayerIntegration()
{
    const collision::World empty;
    player::Settings settings;
    settings.collision = body;
    Camera free{};
    player::update(free, {1, 1}, 1, settings, &empty);
    check(near(std::hypot(free.x, free.y), 300), "Collision integration changed diagonal speed");
    const collision::World wall{{{100, -1000}, {100, 1000}}};
    Camera camera{};
    player::update(camera, {1, -1}, 1, settings, &wall);
    check(near(camera.x, 90) && near(camera.y, 300 / std::sqrt(2.0)),
          "Player update bypasses collisions or loses strafe");
    const auto initialAngle = camera.angle;
    player::update(camera, {1, 0, 1}, 0.1, settings, &wall);
    check(camera.angle != initialAngle, "Wall blocked camera rotation");
}
void testObjets()
{
    game_lab::Level level{};
    level.objetBank = 1;
    game_lab::Sector sector{};
    sector.gx = 1000; sector.gy = 2000;
    sector.vertices = {{500,500}};
    sector.entitiesA = {{0,0x5004,256,42}};
    level.sectors = {sector,sector};
    const auto resolve = [&](Point before, Point candidate) {
        return collision::constrainObjets(level,0,before,candidate);
    };
    check(resolve({1400,2500},{1500,2500}).x == 1420,"OBJET west collision failed");
    check(resolve({1600,2500},{1500,2500}).x == 1580,"OBJET east collision failed");
    check(resolve({1500,2400},{1500,2500}).y == 2420,"OBJET south collision failed");
    check(resolve({1500,2600},{1500,2500}).y == 2580,"OBJET north collision failed");
    const auto corner = resolve({1400,2400},{1500,2500});
    check(corner.x == 1500 && corner.y == 2420,"OBJET corner did not prioritize Y");
    const auto fractionalContact=resolve({1400,2499.25},{1420.25,2500.75});
    check(fractionalContact.x==1420 && fractionalContact.y==2500.75,
          "OBJET clamped the fractional tangential component");
    const auto tangent=resolve({1420,2550.25},{1420,2551.75});
    check(tangent.x==1420 && tangent.y==2551.75,"OBJET stopped tangential movement");
    level.sectors[0].vertices.push_back({500,350});
    level.sectors[0].entitiesA.push_back({1,0x5004,0,0});
    const auto multiple=resolve({1500,2200},{1500.25,2500});
    check(multiple.x==1500.25 && multiple.y==2270,
          "Second OBJET did not read the corrected candidate in record order");
    level.sectors[0]=sector;
    check(resolve({1400,2500},{1580,2500}).x == 1580,"Upper X bound not excluded");
    check(resolve({1600,2500},{1420,2500}).x == 1580,"Lower X bound not included");
    check(resolve({1500,2400},{1500,2580}).y == 2580,"Upper Y bound not excluded");
    check(resolve({1500,2600},{1500,2420}).y == 2580,"Lower Y bound not included");
    check(resolve({1500,2500},{1501,2501}).x == 1501,"OBJET invented inside-box depenetration");
    check(resolve({1400,2500},{1600,2500}).x == 1600,"OBJET invented swept collision");
    for (auto type : {0x4004,0x7004,0x5007}) {
        level.sectors[0].entitiesA[0].type = std::uint16_t(type);
        check(resolve({1400,2500},{1500,2500}).x == 1500,"Traversable OBJET blocked player");
    }
    level.sectors[0].entitiesA.clear();
    check(resolve({1400,2500},{1500,2500}).x == 1500,"OBJET from another sector blocked player");
    level.sectors[0] = sector;
    sprites::Scene scene;
    scene.objetBank = 1;
    scene.objects.push_back({1500,2500,256,255,21});
    scene.placements.push_back({0,0,0x5004,42});
    check(collision::constrainObjets(level,0,{1300,2500},{1400,2500},&scene).x == 1350,
          "OBJET collision ignored live image extent");
    scene.placements[0].sourceSector = 1;
    check(collision::constrainObjets(level,0,{1300,2500},{1400,2500},&scene).x == 1400,
          "OBJET live image matched wrong sector");
    for (unsigned bank = 1; bank <= 10; ++bank)
        check(collision::objetExtent(bank,0) == (bank == 9 ? 80u : bank == 10 ? 120u : 200u),
              "OBJET bank table selection failed");
    check(collision::objetExtent(0,0) == 0 && collision::objetExtent(11,0) == 0 &&
          collision::objetExtent(2,21) == 0,"Invalid OBJET lookup accepted");

    double observedCandidate = 0;
    const player::MovementConstraint constraint = [&](const Camera& before, Camera& after) {
        observedCandidate = after.x;
        const auto p = resolve({before.x,before.y},{after.x,after.y});
        after.x = p.x; after.y = p.y;
    };
    player::Settings settings;
    settings.moveSpeed = 1800; settings.collision = body;
    const collision::World walls{{{1450,2000},{1450,3000}}};
    Camera camera{1400,2500,0};
    player::update(camera,{1,0},1.0/18,settings,&walls,true,{},constraint);
    check(observedCandidate == 1500 && camera.x == 1420 && camera.angle == 0,
          "OBJET correction was not applied before walls");
    for (bool run : {false,true}) {
        Camera moving{1400,2500,0};
        player::Input input{1,0}; input.run = run; input.crouch = true;
        player::update(moving,input,0.5,{},nullptr,true,{},constraint);
        check(moving.x == 1420 && moving.postureHeight == 100,"OBJET broke running/crouching");
    }
    for (double dt : {0.001,0.004,0.016,0.033}) {
        Camera sliding{1420,2480.25,0};
        player::update(sliding,{1,-1},dt,{},nullptr,true,{},constraint);
        check(sliding.x==1420 && std::abs(sliding.y-(2480.25+300*dt/std::sqrt(2.0)))<1e-9,
              "OBJET collision lost fractional sliding at an alternate timestep");
    }
    Camera recoil{1590,2500,0};
    player::recoil(recoil,{},body,constraint);
    check(recoil.x == 1580 && recoil.y == 2500 && recoil.angle == 0,
          "Damage recoil bypassed OBJET collision");
}

void originalObjets(const std::filesystem::path& path)
{
    const auto banks = game_lab::loadBankTable(path);
    std::size_t total = 0, placements = 0;
    for (const auto& entry : banks) {
        const auto level = game_lab::loadLevel(path,entry.zoneIndex).level;
        std::vector<unsigned> indices(256);
        for (const auto& sector : level.sectors) for (const auto& entity : sector.entitiesA) {
            ++placements;
            const auto extent=collision::objetExtent(level.objetBank,entity.type&255);
            const bool solid=(entity.type&0x3000)==0x1000 && extent>0;
            game_lab::Level isolated{};
            isolated.objetBank=level.objetBank;
            isolated.sectors.resize(1);
            auto& objectSector=isolated.sectors.front();
            objectSector.gx=sector.gx; objectSector.gy=sector.gy;
            objectSector.vertices={sector.vertices[entity.vertex]};
            objectSector.entitiesA={{0,entity.type,entity.param1,entity.param2}};
            const Point center{double(sector.gx)+objectSector.vertices[0].x,
                               double(sector.gy)+objectSector.vertices[0].y};
            const Point before{center.x-extent-1,center.y};
            const Point expected{solid?center.x-extent:center.x,center.y};
            const auto player=collision::constrainObjets(isolated,0,before,center);
            game_lab::EnemyRecord enemy{};
            // Use the other side to keep the local coordinate within unsigned range.
            enemy.x=std::uint16_t(objectSector.vertices[0].x+extent+1);
            enemy.y=objectSector.vertices[0].y;
            enemy.angle=360;
            const auto moved=collision::constrainEnemyObjets(isolated,0,enemy,center,nullptr);
            check(player.x==expected.x && player.y==expected.y &&
                  moved.x==(solid?center.x+extent:center.x) && moved.y==center.y &&
                  enemy.angle==(solid?440:360),"Original OBJET placement collision differs from flags and extent");
            if (solid) { ++indices[entity.type&255]; ++total; }
        }
        std::cout << "Level " << entry.zoneIndex+1 << " OBJET" << level.objetBank << " solid index:count";
        for (unsigned i = 0; i < indices.size(); ++i) if (indices[i]) std::cout << ' ' << i << ':' << indices[i];
        std::cout << '\n';
    }
    check(banks.size()==30 && placements==971 && total==732,"Original OBJET inventory changed");
    std::cout << "Effective OBJET obstacles: " << total << " / " << placements << '\n';

    const auto level22=game_lab::loadLevel(path,21).level;
    const Point start{11250,2090};
    const auto source=collision::locate(level22,start,10);
    const auto candidate=collision::move(collision::staticWorld(level22),start,{0,30});
    const auto destination=collision::locate(level22,candidate,10);
    check(source && source->sector==10 && destination && destination->sector==11 &&
          candidate.x==11250 && candidate.y==2120,"Level 22 open-portal fixture changed");
    game_lab::EnemyRecord enemy{};
    enemy.sector=10;
    enemy.x=450; enemy.y=1090; enemy.angle=180;
    const auto corrected=collision::constrainEnemyObjets(level22,destination->sector,enemy,candidate,nullptr);
    check(corrected.x==11250 && corrected.y==2500 && enemy.angle==260,
          "Level 22 OBJET selected the wrong face after the sector change");
}
} // namespace

int main(int argc, char* argv[])
{
    try {
        if (argc==2 && std::string(argv[1])=="--objets") {
            testObjets();
            std::cout << "Player OBJET collision tests passed\n";
            return 0;
        }
        testWallAndSliding();
        testCorners();
        testObliqueAndEndpoints();
        testFastMotionAndTime();
        testPortalsAndGeometry();
        testPlayerIntegration();
        testObjets();
        if (argc == 2) originalObjets(argv[1]);
        std::cout << "Static swept circle collisions, sliding, endpoints, portals and player integration passed.\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
