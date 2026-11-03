#pragma once
#include "sprite_scene.h"
#include "player_collision.h"
#include "camera.h"
#include <array>

namespace enemies {
// One original world-death slot and an independent global GOUTTE timer.
class DeathEffects {
public:
    void reset();
    void begin(collision::Point origin, double elevation, const Camera& camera);
    void step(const Camera& camera, const collision::World& walls);
    void compose(const Camera& camera, const sprites::Bank& bank);
    const std::vector<sprites::Instance>& objects() const { return objects_; }
    std::optional<unsigned> slimePhase() const { return slime_; }
    bool active() const { return phase_.has_value(); }
private:
    struct Fragment { collision::Point position{}; double z=0; int image=-1; };
    std::optional<unsigned> phase_, slime_;
    collision::Point origin_{};
    double elevation_=0, heading_=0;
    std::array<Fragment,4> fragments_{};
    std::vector<sprites::Instance> objects_;
    void splashNear(collision::Point position, const Camera& camera);
};
}
