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

void testHeadbob()
{
    constexpr double tick=1./18;
    Camera c{};
    constexpr int heights[]={170,173,174,175,174,173,170,165};
    for(int height:heights) {
        check(player::update(c,{1,0},tick)==0 && c.viewHeight()==height,"Walking bob sequence");
    }
    check(player::update(c,{1,0},tick)==1 && c.bobPhase==28,"Enabled cycle transition");
    const auto phase=c.bobPhase; const auto height=c.viewHeight();
    check(player::update(c,{},1)==0 && c.bobPhase==phase,"Stopped phase");
    check(player::update(c,{0,0,1,100},1)==0 && c.viewHeight()==height,"Turning bob");
    Camera silent{};
    check(player::update(silent,{1,0},9*tick,{},nullptr,false)==1 && silent.bobPhase==7,"Disabled cycle transition");
    Camera run{}; player::update(run,{1,0,0,0,true},tick);
    check(run.bobPhase==32 && run.viewHeight()==173,"Running increment");
    Camera strafe{};player::update(strafe,{1,0},4*tick);
    check(player::update(strafe,{0,1},20*tick)==0 && strafe.bobPhase==16,"Strafe reset");
    Camera crouch{};player::update(crouch,{1,0,0,0,false,true},7*tick);
    check(crouch.postureHeight==100 && crouch.viewHeight()==105,"Crouched bob");
    const collision::World wall{{{100,-1000},{100,1000}}};
    Camera blocked{};player::update(blocked,{1,0},1,{},&wall);
    const auto stopped=blocked.bobPhase;
    check(player::update(blocked,{1,0},1,{},&wall)==0 && blocked.bobPhase==stopped,"Wall bob");
    Camera constrained{};
    player::update(constrained,{1,0},1,{},nullptr,true,
        [](const Camera& before,Camera& after){after.x=before.x;after.y=before.y;});
    check(constrained.bobPhase==0,"Final enemy constraint ignored");
    Camera recoil{};player::update(recoil,{1,0},tick);
    const auto recoilPhase=recoil.bobPhase;
    player::recoil(recoil,{});
    player::Input reaction{};reaction.blockMovement=true;reaction.crouch=true;
    check(player::update(recoil,reaction,tick)==0 && recoil.bobPhase==recoilPhase &&
          recoil.viewHeight()==160,"Recoil changed gait or posture sum");
    for(bool running:{false,true}) for(int fps:{15,30,60,120,144,240}) {
        Camera whole{},split{};player::Input input{1,0,0,0,running};
        const auto expected=player::update(whole,input,4);
        unsigned events=0;
        for(int f=0;f<4*fps;++f) events+=player::update(split,input,1./fps);
        check(events==expected && split.bobPhase==whole.bobPhase &&
              split.viewHeight()==whole.viewHeight(),"Bob depends on render FPS");
    }
    Camera partial{}; player::update(partial,{1,0},tick/2);
    check(partial.bobPhase==0,"Phase advanced outside fixed tick");
    player::update(partial,{},tick/2);
    check(partial.bobPhase==16,"Partial tick movement lost");
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
        testRotation(); testRunning(); testPosture(); testHeadbob();
        std::cout << "Planar WASD, normalized diagonals, timing and rotation passed (no SDL).\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
