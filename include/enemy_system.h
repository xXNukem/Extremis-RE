#pragma once

#include "sprite_scene.h"
#include "player_collision.h"
#include "player_vitals.h"
#include "camera.h"
#include "enemy_effects.h"
#include "door.h"
#include <functional>

namespace enemies {
struct ShotHit { std::size_t enemy; double distance; };
class System {
public:
    void load(const game_lab::Level& level, const std::filesystem::path& directory);
    void update(const game_lab::Level& level, const Camera& player, const collision::World& walls,
                player::Vitals& vitals, double seconds, bool active,
                const std::function<void(double)>& movePlayer = {},
                const std::function<void()>& recoilPlayer = {},
                const std::vector<doors::Door>& dynamicDoors = {});
    std::optional<ShotHit> trace(const Camera& view, collision::Point origin,
                                collision::Point direction, double range) const;
    void hit(std::size_t enemy, unsigned weapon, double angle);
    void constrainPlayer(const game_lab::Level& level, const Camera& before, Camera& after,
                         const collision::World& walls) const;
    std::vector<unsigned> takeSounds();
    // DOS connected-sector membership, shared with behavior 2 activation.
    bool blocksInterface(const game_lab::Level& level, const Camera& camera,
                         const std::vector<doors::Door>& dynamicDoors = {}) const;
    // MONS9 death requests the DOS ending; presentation remains with the caller.
    bool terminalDefeated() const { return terminalDefeated_; }
    bool reacting() const { return reaction_ != 0; }
    unsigned simulationTicks() const { return ticks_; }
    const DeathEffects& effects() const { return effects_; }
    const sprites::Scene& scene() const { return scene_; }
    const std::vector<game_lab::EnemyRecord>& records() const { return records_; }
private:
    sprites::Scene scene_;
    DeathEffects effects_;
    std::vector<game_lab::EnemyRecord> records_;
    std::vector<collision::Point> origins_;
    std::vector<std::uint8_t> masks_;
    std::vector<unsigned> sounds_;
    std::vector<unsigned> recoil5_;
    double accumulator_=0;
    unsigned reaction_=0, attackSound_=0, ticks_=0;
    bool wasActive_=false;
    unsigned bank_=1;
    double speed_=30, baseElevation_=0;
    bool terminalDefeated_=false;
    bool directionalCycle_=false;
    double baseHeight_=270;
    void rebuildScene();
    void advance5(std::size_t index, const game_lab::Level& level, const Camera& camera,
                  const collision::World& walls, std::optional<collision::Location> playerLocation);
    void displace5(std::size_t index, const game_lab::Level& level,
                   const collision::World& walls, double distance, bool crossSector);
    void step(const game_lab::Level& level, const Camera& player, const collision::World& walls,
              player::Vitals& vitals, const std::function<void()>& recoilPlayer,
              const std::vector<doors::Door>& dynamicDoors);
    void move(std::size_t index, const game_lab::Level& level, collision::Point destination,
              const Camera& camera, const collision::World& walls);
    void advance(std::size_t index, const game_lab::Level& level, const Camera& player,
                 const collision::World& walls, std::optional<collision::Location> playerLocation);
};
} // namespace enemies
