#pragma once

#include "player_inventory.h"
#include "camera.h"
#include "player_collision.h"
#include "sprite_scene.h"
#include <array>

namespace enemies { class System; }

namespace player {
struct ProjectileFrame { int width, height, anchorX, anchorY, worldHeight; };
struct WeaponDefinition {
    bool automatic;
    unsigned wait;
    int speed, aimX, aimY, gunX, gunY;
    bool alternating;
    std::array<std::array<int,2>,3> muzzle;
    std::array<int,3> lateral;
    std::array<ProjectileFrame,6> frames;
    std::array<unsigned,6> stages;
};
// EXTREMIS.EXE file 0x56d7, six descriptors of 112 bytes. No guessed FPS values.
inline constexpr std::array<WeaponDefinition,6> weaponDefinitions{{
    {false,4,80,150,48,148,122,false,{{{5,-2},{0,0},{0,0}}},{0,0,0},
     {{{5,9,2,3,10},{13,13,6,6,40},{10,11,4,5,33},{5,6,2,3,18},{},{}}},{0,1,2,1,2,3}},
    {true,3,80,150,48,144,113,false,{{{10,-5},{0,0},{0,0}}},{0,0,0},
     {{{5,9,2,3,10},{13,13,6,6,40},{10,11,4,5,33},{5,6,2,3,18},{},{}}},{0,1,2,1,2,3}},
    {true,2,80,147,45,147,116,true,{{{0,-7},{6,-10},{13,-7}}},{5,0,-5},
     {{{5,9,2,3,10},{13,13,6,6,40},{10,11,4,5,33},{5,6,2,3,18},{},{}}},{0,1,2,1,2,3}},
    {true,3,40,150,48,147,100,false,{{{6,-2},{0,0},{0,0}}},{0,0,0},
     {{{15,14,7,7,10},{26,25,13,13,120},{19,20,9,10,96},{16,14,7,7,67},{},{}}},{0,1,2,1,2,3}},
    {false,6,80,142,40,137,117,false,{{{9,-5},{0,0},{0,0}}},{0,0,0},
     {{{13,29,6,12,30},{61,61,31,34,250},{45,40,16,25,163},{32,26,17,10,106},{},{}}},{0,1,2,1,2,3}},
    {true,1,100,148,46,148,103,false,{{{5,-9},{0,0},{0,0}}},{0,0,0},
     {{{7,19,3,14,20},{11,9,5,4,34},{18,14,8,6,53},{24,17,11,8,64},{26,19,13,9,72},{28,21,14,10,80}}},{0,1,2,3,4,5}}
}};

struct FireTrigger { bool held=false, pressed=false; };
enum class FireResult { None, Fired, Empty, Full };

// One update per active game iteration, as in DOS; rendering owns no combat state.
class WeaponFire {
public:
    void loadGraphics(const std::filesystem::path& directory, unsigned paletteBank);
    FireResult update(const Camera& camera, const collision::World& walls, Inventory& inventory,
                      FireTrigger trigger, bool active, double seconds, enemies::System* enemies = nullptr);
    bool firing() const { return firing_; }
    unsigned barrelPhase() const { return phase_; }
    unsigned estimatedRate() const { return rate_; }
    unsigned cooldown() const { return cooldown_; }
    std::size_t activeProjectiles() const;
    const sprites::Scene& scene() const { return scene_; }
private:
    struct Projectile {
        unsigned state=0, weapon=0, phase=0;
        double x=0, y=0, angle=0;
        double traveled=0, visualSide=0, visualDrop=0;
        double launchHeight=Camera::eyeHeight;
    };
    std::array<Projectile,20> projectiles_{};
    sprites::Scene scene_;
    unsigned rate_=9, frames_=0, cooldown_=0, phase_=0, weapon_=0;
    double rateTime_=0;
    bool firing_=false, blocked_=true, pending_=false, wasActive_=false;
    void measureRate(double seconds);
    FireResult attempt(const Camera& camera, Inventory& inventory);
    void advanceProjectiles(const Camera& camera, const collision::World& walls, enemies::System* enemies);
};
}
