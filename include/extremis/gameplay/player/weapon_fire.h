#pragma once

#include <cstddef>
#include <filesystem>
#include "extremis/gameplay/player/player_inventory.h"
#include "extremis/gameplay/player/weapon_definitions.h"
#include "extremis/gameplay/player/camera.h"
#include "extremis/world/player_collision.h"
#include "extremis/render/first_person/sprites/sprite_scene.h"
#include <array>

namespace enemies { class System; }

namespace player {
struct FireTrigger { bool held=false, pressed=false; };
enum class FireResult { None, Fired, Empty, Full };

// Receives monotonic elapsed time from the gameplay loop; rendering reads the latest state.
class WeaponFire {
public:
    void loadGraphics(const std::filesystem::path& directory, unsigned paletteBank);
    FireResult update(const Camera& camera, const collision::World& walls, Inventory& inventory,
                      FireTrigger trigger, bool active, double seconds, enemies::System* enemies = nullptr,
                      const collision::World& doorLeaves = {});
    bool firing() const;
    unsigned barrelPhase() const;
    unsigned estimatedRate() const;
    unsigned cooldown() const;
    std::size_t activeProjectiles() const;
    const sprites::Scene& scene() const;
private:
    struct Projectile {
        unsigned state=0, weapon=0, phase=0;
        double x=0, y=0, angle=0;
        double traveled=0, visualSide=0, visualDrop=0;
        double launchHeight=Camera::eyeHeight;
        bool wallImpact=false;
    };
    std::array<Projectile,20> projectiles_{};
    sprites::Scene scene_;
    unsigned rate_=9, frames_=0, cooldown_=0, phase_=0, weapon_=0;
    double rateTime_=0, projectileTime_=0, flashTime_=0;
    bool blocked_=true, pending_=false, wasActive_=false;
    void measureRate(double seconds);
    FireResult attempt(const Camera& camera, Inventory& inventory);
    void advanceProjectiles(const Camera& camera, const collision::World& walls, enemies::System* enemies,
                            const collision::World& doorLeaves);
};
}
