#include "door_control.h"
#include "player_control.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
constexpr double pi = 3.14159265358979323846;
const collision::Settings body{10, 0.0001, 8};
void check(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}
bool near(double a, double b) { return std::abs(a - b) < 0.001; }

doors::Door doorAt(double x = 100)
{
    return {0, 0, {{{{x, 100}, {x, -100}, 23}, {{x, -100}, {x, 100}, 33}}}};
}

void testDoorCollision()
{
    const collision::World walls;
    for (const double side : {-1.0, 1.0}) {
        Camera camera{100 + side * 50, 0, side < 0 ? 0 : pi};
        std::vector<doors::Door> doors{doorAt()};
        DoorControl control(walls, doors);
        control.prepareFrame(&camera, 0, body);
        auto result = collision::move(control.movementWorld(), {camera.x, camera.y}, {-side * 100000, 0}, body);
        check(near(result.x, 100 + side * 10), "Closed door allows passage from one side or at high speed");
        doors::toggle(doors[0]);
        control.prepareFrame(&camera, 1, body);
        result = collision::move(control.movementWorld(), {camera.x, camera.y}, {-side * 100, 0}, body);
        check(near(result.x, camera.x - side * 100), "Open door blocks passage");
    }
}

void testPartialAperture()
{
    // Frames at y=-100 and y=100 make the usable gap finite.
    const collision::World walls{{{0, -100}, {200, -100}}, {{0, 100}, {200, 100}}};
    for (const double side : {-1.0, 1.0}) {
        std::vector<doors::Door> doors{doorAt()};
        DoorControl control(walls, doors);
        doors[0].state = doors::State::Opening;
        doors[0].progress = 0.05; // 10-unit gap, less than diameter 20.
        Camera camera{100 + side * 50, side < 0 ? 95.0 : -95.0, 0};
        control.prepareFrame(&camera, 0, body);
        auto result = collision::move(control.movementWorld(), {camera.x, camera.y}, {-side * 100, 0}, body);
        check(side < 0 ? result.x < 100 : result.x > 100, "Undersized partial opening admits circle");
        doors[0].progress = 0.3; // 60-unit gap, sufficient for the same body.
        camera.y = side < 0 ? 70 : -70;
        control.prepareFrame(&camera, 0, body);
        result = collision::move(control.movementWorld(), {camera.x, camera.y}, {-side * 100, 0}, body);
        check(near(result.x, camera.x - side * 100), "Sufficient partial opening blocks circle");
    }
}

void testCrossingFaceLatch()
{
    const collision::World walls;
    std::vector<doors::Door> doors{doorAt()};
    doors[0].state = doors::State::Opening;
    doors[0].progress = 0.3;
    DoorControl control(walls, doors);
    Camera camera{50, 70, 0};
    control.prepareFrame(&camera, 0, body);
    camera.x = 105; // Circle has crossed the plane but has not yet cleared it.
    control.prepareFrame(&camera, 0, body);
    auto result = collision::move(control.movementWorld(), {camera.x, camera.y}, {20, 0}, body);
    check(near(result.x, 125), "Changing rendered face traps the crossing player");
    camera.x = 125;
    control.prepareFrame(&camera, 0, body);
    result = collision::move(control.movementWorld(), {camera.x, camera.y}, {-50, 0}, body);
    check(result.x > 100, "Approach face did not reset after clearing plane");
}

void testIndividualUse()
{
    const collision::World walls;
    std::vector<doors::Door> doors{doorAt(), doorAt(180)};
    DoorControl control(walls, doors);
    Camera camera{50, 0, 0};
    control.prepareFrame(&camera, 0, body);
    check(control.interact(camera) == DoorControl::UseResult::Activated &&
          doors[0].state == doors::State::Opening && doors[1].state == doors::State::Closed,
          "One use did not select exactly the nearest door");
    control.prepareFrame(&camera, 0.25, body);
    const double progress = doors[0].progress;
    check(control.interact(camera) == DoorControl::UseResult::Activated &&
          doors[0].state == doors::State::Closing && doors[0].progress == progress,
          "Individual use resets animation instead of reversing it");
    check(control.interact({50, 0, pi}) == DoorControl::UseResult::None,
          "Interaction activated a door behind player");
    check(control.interact({-200, 0, 0}) == DoorControl::UseResult::None,
          "Out-of-range interaction activated door");
    check(control.interact({140, 0, pi}) == DoorControl::UseResult::Activated,
          "Cannot use door from opposite side");
}

void testOcclusionAndAccess()
{
    const collision::World walls{{{75, -100}, {75, 100}}};
    std::vector<doors::Door> doors{doorAt()};
    DoorControl control(walls, doors);
    Camera camera{50, 0, 0};
    control.prepareFrame(&camera, 0, body);
    check(control.interact(camera) == DoorControl::UseResult::None &&
          doors[0].state == doors::State::Closed, "Use passes through solid wall");
    const collision::World empty;
    doors[0].originalState = 0x0400;
    DoorControl locked(empty, doors);
    locked.prepareFrame(&camera, 0, body);
    check(locked.interact(camera) == DoorControl::UseResult::Locked &&
          doors[0].state == doors::State::Closed, "Normal use bypassed original access gate");
    doors::toggleAll(doors);
    locked.prepareFrame(&camera, 1, body);
    check(doors[0].state == doors::State::Open, "O-style debug toggle no longer bypasses access gate");
}

void testSafeClosing()
{
    const collision::World walls;
    for (const double side : {-1.0, 1.0}) {
        std::vector<doors::Door> doors{doorAt()};
        doors[0].state = doors::State::Open;
        doors[0].progress = 1;
        DoorControl control(walls, doors);
        Camera player{100 + side * 5, 0, 0};
        control.prepareFrame(&player, 0, body);
        doors::toggle(doors[0]);
        control.prepareFrame(&player, 100, body); // Whole closure in one very long frame.
        check(doors[0].state == doors::State::Opening && doors[0].progress == 1 &&
              control.movementWorld().empty() && player.x == 100 + side * 5,
              "Closing door crushed, trapped or moved player");
        player.x = 50;
        doors::toggle(doors[0]);
        control.prepareFrame(&player, 1, body);
        check(doors[0].state == doors::State::Closed, "Door cannot close once path is clear");
    }
    std::vector<doors::Door> doors{doorAt()};
    doors[0].state = doors::State::Closing;
    doors[0].progress = 0.8;
    DoorControl control(walls, doors);
    Camera player{95, 50, 0};
    control.prepareFrame(&player, 10, body);
    check(doors[0].progress == 0.8 && doors[0].state == doors::State::Opening,
          "Safety reversal moved a partially closing door before checking its swept region");
    control.prepareFrame(&player, 0.1, body);
    check(near(doors[0].progress, 0.9), "Safety reversal did not continue opening");
}

void testInteractionTrace()
{
    const collision::World empty;
    std::vector<doors::Door> doors{doorAt()};
    doors[0].stateIndex = 12;
    DoorControl control(empty, doors);
    const Camera camera{50, 0, 0};
    control.prepareFrame(&camera, 0, body);
    DoorControl::UseTrace trace;
    control.interact({-200, 0, 0}, 200, &trace);
    check(trace.reason == DoorControl::UseReason::OutOfRange && trace.doorId == 12 &&
          trace.distance == 300 && trace.before == trace.after, "Far-door diagnostic incorrect");
    control.interact({50, 0, pi}, 200, &trace);
    check(trace.reason == DoorControl::UseReason::NoCandidate && !trace.doorId,
          "Trace retained stale candidate for a door behind player");
    doors[0].originalState = 0x0400;
    control.interact(camera, 200, &trace);
    check(trace.reason == DoorControl::UseReason::AccessCondition && trace.doorId == 12 &&
          trace.before == trace.after, "Access diagnostic did not preserve restriction");
    const collision::World wall{{{75, -100}, {75, 100}}};
    DoorControl blocked(wall, doors);
    blocked.prepareFrame(&camera, 0, body);
    blocked.interact(camera, 200, &trace);
    check(trace.reason == DoorControl::UseReason::Occluded && trace.obstacleDistance == 25,
          "Occlusion diagnostic did not identify blocker");
}

void testSharedCollisionResolution()
{
    const collision::World empty;
    std::vector<doors::Door> doors{doorAt()};
    DoorControl control(empty, doors);
    const Camera camera{50, 0, 0};
    for (const double progress : {0.0, 0.25, 0.5}) {
        doors[0].progress = progress;
        doors[0].state = progress == 0 ? doors::State::Closed : doors::State::Opening;
        control.prepareFrame(&camera, 0, body);
        const auto face = doors::visibleFace(doors[0], doors[0].faces[0]);
        const collision::World sameStaticSurface{{{face->a.x, face->a.y}, {face->b.x, face->b.y}}};
        for (const auto motion : {collision::Point{100, -40}, collision::Point{100000, 0},
                                  collision::Point{0, 40}, collision::Point{100, 100}}) {
            const auto fixed = collision::move(sameStaticSurface, {camera.x, camera.y}, motion, body);
            const auto dynamic = collision::move(control.movementWorld(), {camera.x, camera.y}, motion, body);
            check(near(fixed.x, dynamic.x) && near(fixed.y, dynamic.y),
                  "Static and dynamic segments use different collision/slide rules");
        }
    }
    check(near(collision::distanceToSegment({{0, 0}, {0, 100}}, {3, 104}), 5) &&
          near(collision::signedDistanceToLine({{0, 0}, {0, 100}}, {3, 104}), -3),
          "Shared endpoint distance or signed line distance incorrect");
}
void testCardReader()
{
    const collision::World walls;
    auto secured = doorAt(); secured.stateIndex = 7; secured.originalState = 0x0400;
    auto other = doorAt(500); other.stateIndex = 8; other.originalState = 0x0400;
    std::vector<doors::Door> doors{secured,other};
    DoorControl control(walls,doors);
    using Result = DoorControl::CardResult;
    check(control.useCardReader(0x0107,std::nullopt) == Result::NoCard,"Missing card unlocked door");
    check(control.useCardReader(0x0107,0x0001) == Result::Rejected,"E card unlocked C reader");
    check(control.useCardReader(0x0107,0x1002) == Result::Rejected,"Incorrect identifier unlocked door");
    check(control.useCardReader(0x0109,0x1001) == Result::InvalidTarget,"Unknown door modified another target");
    check(control.useCardReader(0x0107,0x1001) == Result::Unlocked &&
          doors[0].originalState == 0 && doors[0].state == doors::State::Closed &&
          doors[1].originalState == 0x0400,"Card opened door or changed unrelated security");
    check(control.useCardReader(0x0107,0x1001) == Result::AlreadyDisabled &&
          doors[0].originalState == 0,"Repeated reader use re-locked door");
    check(control.interact({50,0,0}) == DoorControl::UseResult::Activated,
          "Unlocked door still rejects ordinary use");
}

void testOriginalSecurity(const std::filesystem::path& path)
{
    const auto level = game_lab::loadLevel1(path).level;
    auto doors = doors::closedDoors(level);
    const auto original = doors;
    const auto walls = collision::staticWorld(level);
    DoorControl control(walls,doors);
    unsigned locked = 0;
    for (const auto& door : doors) if (door.originalState & 0x0400) {
        ++locked;
        check(door.stateIndex == 7,"Unexpected original secured door ID");
    }
    check(locked == 1 && level.sectors[7].walls[17].type == 0x3308 &&
          level.sectors[7].walls[17].extra == 0x0107,"Original reader/door association changed");
    check(control.useCardReader(0x0107,0x1001) == DoorControl::CardResult::Unlocked,
          "Actual level-1 C001 reader did not unlock its door");
    for (std::size_t i = 0; i < doors.size(); ++i)
        check(doors[i].originalState == (original[i].originalState & ~0x0400),
              "Original security transfer changed other door state bits");
    std::cout << "Actual level-1 security: reader 7/17 unlocks only door 7.\n";
}
} // namespace

int main(int argc, char* argv[])
{
    try {
        testDoorCollision();
        testPartialAperture();
        testCrossingFaceLatch();
        testIndividualUse();
        testOcclusionAndAccess();
        testSafeClosing();
        testInteractionTrace();
        testSharedCollisionResolution();
        testCardReader();
        if (argc == 2) testOriginalSecurity(argv[1]);
        std::cout << "Dynamic door collisions, individual use, access gate and safe closing passed.\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
