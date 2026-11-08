#include "player_control.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
constexpr double pi = 3.14159265358979323846;
void check(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}
bool near(double a, double b) { return std::abs(a - b) < 1e-9; }

void testDirections()
{
    const player::Settings settings{};
    const Camera origin{3500, 900, 0};
    for (const double angle : {0.0, pi / 2, pi, -pi / 2, 0.37}) {
        for (const auto input : {player::Input{1, 0}, player::Input{-1, 0},
                                 player::Input{0, -1}, player::Input{0, 1}}) {
            auto camera = origin;
            camera.angle = angle;
            player::update(camera, input, 1);
            const double x = settings.moveSpeed * (std::cos(angle) * input.forward +
                                                   std::sin(angle) * input.right);
            const double y = settings.moveSpeed * (std::sin(angle) * input.forward -
                                                   std::cos(angle) * input.right);
            check(near(camera.x, origin.x + x) && near(camera.y, origin.y + y),
                  "Movement direction does not match camera/screen-right convention");
        }
    }
    auto idle = origin;
    player::update(idle, {}, 1);
    check(idle.x == origin.x && idle.y == origin.y && idle.angle == origin.angle,
          "Idle input moved the initial camera");
}

void testDiagonalAndTime()
{
    for (const double forward : {-1.0, 1.0}) {
        for (const double right : {-1.0, 1.0}) {
            Camera single{0, 0, 0.37}, sixty = single, uneven = single;
            const player::Input input{forward, right};
            player::update(single, input, 1);
            for (int i = 0; i < 60; ++i) player::update(sixty, input, 1.0 / 60);
            for (const double dt : {0.13, 0.02, 0.5, 0.35}) player::update(uneven, input, dt);
            check(near(std::hypot(single.x, single.y), 300) &&
                  near(single.x, sixty.x) && near(single.y, sixty.y) &&
                  near(single.x, uneven.x) && near(single.y, uneven.y),
                  "Diagonal speed or movement depends on FPS");
        }
    }
}

void testRunning()
{
    for(double forward:{-1.,0.,1.}) for(double right:{-1.,0.,1.}) {
        Camera walking{}, running{}, split{};
        player::update(walking,{forward,right},1);
        const player::Input run{forward,right,0,0,true};
        player::update(running,run,1);
        for(int i=0;i<60;++i) player::update(split,run,1./60);
        check(near(running.x,walking.x*2) && near(running.y,walking.y*2) &&
              near(running.x,split.x) && near(running.y,split.y),"Run ratio/diagonal/FPS differs");
    }
    const collision::World wall{{{100,-1000},{100,1000}}};
    Camera camera{}; player::update(camera,{1,0,0,0,true},1,{},&wall);
    check(camera.x<70.001,"Running tunneled through collision");
    Camera angle{}; player::update(angle,{0,0,1,0,true},1);
    check(near(angle.angle,1.5),"Running changed turn speed");
}
void testPosture()
{
    Camera camera{};
    player::Input crouch{}; crouch.crouch=true;
    for(int i=1;i<=7;++i) {
        player::update(camera,crouch,1./18);
        check(near(camera.postureHeight,std::max(100.,165.-10*i)),"Crouch transition differs");
    }
    player::update(camera,crouch,1);
    check(camera.postureHeight==100,"Held crouch did not stay down");
    for(int i=1;i<=7;++i) {
        player::update(camera,{},1./18);
        check(near(camera.postureHeight,std::min(165.,100.+10*i)),"Stand transition differs");
    }
    for(int fps : {30,60,120}) {
        Camera split{},whole{};
        const player::Input run{1,0,0,0,true,true};
        player::update(whole,run,1);
        for(int i=0;i<fps;++i) player::update(split,run,1./fps);
        check(near(split.x,whole.x) && split.postureHeight==100 && split.movementStep==10,
              "Crouch braking depends on rendering FPS");
        const double before=split.x;
        player::update(split,run,1);
        check(near(split.x-before,300),"Held run accelerated crouched player");
    }
    Camera braking{};
    player::update(braking,{1,0,0,0,true},1./18);
    player::update(braking,{1,0,0,0,true,true},2./18);
    check(braking.postureHeight==145 && braking.movementStep==18,"First braking step differs");
    player::update(braking,{1,0,0,0,true,true},1./18);
    check(braking.movementStep==16,"Braking is not two units per tick");
    const collision::World wall{{{100,-1000},{100,1000}}};
    Camera standing{},low{};low.postureHeight=100;
    player::update(standing,{1,0},1,{},&wall);
    player::update(low,{1,0,0,0,false,true},1,{},&wall);
    check(near(standing.x,low.x) && low.x<70.001,"Posture changed planar collision radius");
}

void testRotation()
{
    Camera left{}, right{}, mouse{}, splitMouse{}, splitKeys{};
    player::update(left, {0, 0, 1, 0}, 1);
    player::update(right, {0, 0, -1, 0}, 1);
    check(near(left.angle, 1.5) && near(right.angle, -1.5), "Arrow rotation changed");
    for (int i = 0; i < 60; ++i) player::update(splitKeys, {0, 0, 1, 0}, 1.0 / 60);
    check(near(splitKeys.angle, left.angle), "Keyboard rotation depends on FPS");
    player::update(mouse, {0, 0, 0, 100}, 0.01);
    for (int i = 0; i < 10; ++i) player::update(splitMouse, {0, 0, 0, 10}, 0.1);
    check(near(mouse.angle, -0.3) && near(mouse.angle, splitMouse.angle),
          "Mouse direction or sensitivity depends on elapsed time");
    player::update(mouse, {0, 0, 0, -100}, 0);
    check(near(mouse.angle, 0), "Mouse reversal failed");
    Camera rotated{0, 0, pi / 2};
    player::update(rotated, {1, 0}, 1);
    check(near(rotated.x, 0) && near(rotated.y, 300), "Forward ignores current heading");
}
} // namespace

int main()
{
    try {
        testDirections();
        testDiagonalAndTime();
        testRotation(); testRunning(); testPosture();
        std::cout << "Planar WASD, normalized diagonals, timing and rotation passed (no SDL).\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
