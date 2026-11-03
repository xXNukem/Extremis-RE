#include "footstep_cadence.h"
#include "player_control.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void check(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

double move(Camera& camera, const player::Input& input, double seconds,
            const collision::World* world = nullptr)
{
    const auto before = camera;
    player::update(camera, input, seconds, {}, world);
    return std::hypot(camera.x - before.x, camera.y - before.y);
}

void originalReference()
{
    audio::FootstepCadence cadence;
    unsigned phase = 0;
    for (unsigned frame = 0; frame < 300; ++frame) {
        phase += 16;
        const bool requested = phase >= 144;
        if (requested) phase = 7 * 4; // EAX returned by original 0x13103.
        check(cadence.advance(10) == unsigned(requested),
              "Distance cadence differs from original 18 FPS reference gait");
    }
    audio::FootstepCadence boundaries;
    check(boundaries.advance(89) == 0 && boundaries.advance(1) == 1 &&
          boundaries.advance(79) == 0 && boundaries.advance(1) == 1,
          "Original first/subsequent step spacing changed");
    check(boundaries.advance(0) == 0 && boundaries.advance(-10) == 0 &&
          boundaries.advance(std::numeric_limits<double>::quiet_NaN()) == 0 &&
          boundaries.advance(std::numeric_limits<double>::infinity()) == 0,
          "No/invalid displacement triggered a step");
}

void frameIndependence()
{
    for (int fps : {1, 15, 30, 60, 144, 240}) {
        for (const auto input : {player::Input{1,0}, player::Input{-1,0},
                                 player::Input{0,1}, player::Input{0,-1}, player::Input{1,1}}) {
            Camera camera{};
            audio::FootstepCadence cadence;
            std::uint64_t count = 0;
            for (int frame = 0; frame < 4 * fps; ++frame)
                count += cadence.advance(move(camera, input, 1.0 / fps));
            check(count == 14 && std::abs(std::hypot(camera.x, camera.y)-1200) < 1e-7,
                  "FPS or diagonal input changed distance/step cadence");
        }
    }
    audio::FootstepCadence whole, split;
    const auto count = whole.advance(1200);
    std::uint64_t divided = 0;
    for (double distance : {13., 2., 500., 35., 200., 450.}) divided += split.advance(distance);
    check(count == divided && whole.advance(10) == split.advance(10),
          "Long frames lost cadence phase or threshold crossings");
}

void effectiveMovement()
{
    const collision::World wall{{{100,-1000},{100,1000}}};
    Camera camera{};
    audio::FootstepCadence cadence;
    check(cadence.advance(move(camera,{1,0},1,&wall)) == 0, "Wall approach fixture invalid");
    for (int frame = 0; frame < 200; ++frame) {
        const auto distance = move(camera,{1,0},1.0/60,&wall);
        check(distance < 1e-9 && cadence.advance(distance) == 0,
              "Blocked input produced footsteps");
    }
    const auto position = camera;
    check(cadence.advance(move(camera,{0,0,1,100},1,&wall)) == 0 &&
          std::hypot(camera.x-position.x,camera.y-position.y) < 1e-9,
          "Turning/mouse alone produced footsteps");
    camera.angle = 0;
    std::uint64_t count = 0;
    for (int frame = 0; frame < 60; ++frame)
        count += cadence.advance(move(camera,{0,1},1.0/60,&wall));
    check(count > 0, "Effective parallel sliding did not advance footsteps");
    check(cadence.advance(move(camera,{},1,&wall)) == 0, "Standing still produced a step");
}
} // namespace

int main()
{
    try {
        originalReference(); frameIndependence(); effectiveMovement();
        std::cout << "Original gait reference, distance cadence, collisions and FPS independence passed.\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
