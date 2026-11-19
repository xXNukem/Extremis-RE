#pragma once
#include "extremis/render/first_person/sprites/sprite_scene.h"
#include "extremis/world/player_collision.h"
#include "extremis/gameplay/player/camera.h"
#include <array>

namespace enemies {
// One original world-death slot and an independent global GOUTTE timer.
class DeathEffects {
public:
    void reset();
    void begin(collision::Point origin, double elevation, const Camera& camera,
               double baseHeight=270, bool bodyFragments=true, unsigned bank=1);
    void step(const Camera& camera, const collision::World& walls);
    void compose(const Camera& camera, const sprites::Bank& bank);
    const std::vector<sprites::Instance>& objects() const;
    std::optional<unsigned> slimePhase() const;
    bool active() const;
private:
    struct Fragment { collision::Point position{}; double z=0; int image=-1; };
    std::optional<unsigned> phase_, slime_;
    collision::Point origin_{};
    double elevation_=0, heading_=0, baseHeight_=270;
    bool bodyFragments_=true;
    unsigned bank_=1;
    std::array<Fragment,4> fragments_{};
    std::vector<sprites::Instance> objects_;
    void splashNear(collision::Point position, const Camera& camera);
};
}
