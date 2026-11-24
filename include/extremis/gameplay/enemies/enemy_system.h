#pragma once

#include "extremis/render/first_person/sprites/sprite_scene.h"
#include "extremis/world/player_collision.h"
#include "extremis/gameplay/player/player_vitals.h"
#include "extremis/gameplay/player/camera.h"
#include "extremis/gameplay/enemies/enemy_effects.h"
#include "extremis/world/door.h"
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
                const std::vector<doors::Door>& dynamicDoors = {},
                const sprites::Scene* objets = nullptr);
    std::optional<ShotHit> trace(const Camera& view, collision::Point origin,
                                collision::Point direction, double range) const;
    void hit(std::size_t enemy, unsigned weapon, double angle);
    void updateGenerators(const game_lab::Level& level, const Camera& camera, std::uint16_t ticks,
                          std::uint16_t& campaignAccumulator);
    std::vector<unsigned> blast(game_lab::Level& level, game_lab::Vertex origin);
    void constrainPlayer(const game_lab::Level& level, const Camera& before, Camera& after,
                         const collision::World& walls) const;
    std::vector<unsigned> takeSounds();
    // DOS connected-sector membership, shared with behavior 2 activation.
    bool blocksInterface(const game_lab::Level& level, const Camera& camera,
                         const std::vector<doors::Door>& dynamicDoors = {}) const;
    // MONS9 death requests the DOS ending; presentation remains with the caller.
    bool terminalDefeated() const;
    void clearPlayerImpact(); // Shuttle common cleanup, EXE 0xc7bc.
    bool reacting() const;
    unsigned simulationTicks() const;
    const DeathEffects& effects() const;
    const sprites::Scene& scene() const;
    const std::vector<game_lab::EnemyRecord>& records() const;
private:
    sprites::Scene scene_;
    // One entry per visible instance, in scene order. Removed records have no entry.
    std::vector<std::uint16_t> objectRecords_;
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
                  const collision::World& walls, std::optional<collision::Location> playerLocation,
                  const std::vector<doors::Door>& dynamicDoors, const sprites::Scene* objets);
    void displace5(std::size_t index, const game_lab::Level& level,
                   const collision::World& walls, double distance, bool crossSector, bool connectionMode,
                   const std::vector<doors::Door>& dynamicDoors, const sprites::Scene* objets);
    void step(const game_lab::Level& level, const Camera& player, const collision::World& walls,
              player::Vitals& vitals, const std::function<void()>& recoilPlayer,
              const std::vector<doors::Door>& dynamicDoors, const sprites::Scene* objets);
    void move(std::size_t index, const game_lab::Level& level, collision::Point destination,
              const Camera& camera, const collision::World& walls, bool connectionMode,
              const std::vector<doors::Door>& dynamicDoors, const sprites::Scene* objets);
    void advance(std::size_t index, const game_lab::Level& level, const Camera& player,
                 const collision::World& walls, std::optional<collision::Location> playerLocation,
                 const std::vector<doors::Door>& dynamicDoors, const sprites::Scene* objets);
};
} // namespace enemies
