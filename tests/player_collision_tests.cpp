#include "player_control.h"

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
} // namespace

int main()
{
    try {
        testWallAndSliding();
        testCorners();
        testObliqueAndEndpoints();
        testFastMotionAndTime();
        testPortalsAndGeometry();
        testPlayerIntegration();
        std::cout << "Static swept circle collisions, sliding, endpoints, portals and player integration passed.\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
