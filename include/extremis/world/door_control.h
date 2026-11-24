#pragma once

#include <cstdint>
#include <optional>
#include <vector>
#include "extremis/gameplay/player/camera.h"
#include "extremis/world/door.h"
#include "extremis/world/door_clock.h"
#include "extremis/world/player_collision.h"

namespace doors {
// Apply native clearance against recognized, non-passable door faces before OBJET.
collision::Point moveEnemy(const game_lab::Level& level, const std::vector<Door>& doors,
                           const collision::World& walls, unsigned sectorIndex,
                           collision::Point before, collision::Point displacement,
                           bool connectionMode);
}

// Connection movement runs once after the player's proposed movement, not per
// enemy/posture substep. State words belong exclusively to the borrowed level.
class DoorControl {
public:
    using Time = doors::dos::Counter::Time;
    using Clock = doors::dos::Counter::Clock;
    DoorControl(const collision::World& staticWorld, const std::vector<doors::Door>& doors,
                game_lab::Level& level, Time now = Clock::now());
    void resetLevel(Time now = Clock::now());
    bool beginFrame(bool active, Time now = Clock::now());
    // Returns whether the ordinary request encountered an access lock.
    bool constrainPlayer(const Camera& before, Camera& proposed);
    void updateDoors(Time now = Clock::now());
    void resetCounter(Time now = Clock::now());
    // Share the gameplay PIT word without changing its reset/suspension lifecycle.
    std::uint16_t gameplayTicks(Time now = Clock::now());
    void rebuildWorld();
    enum class CardResult { Unlocked, AlreadyDisabled, Rejected, NoCard, InvalidTarget };
    CardResult useCardReader(std::uint16_t parameter, std::optional<std::uint16_t> card);
    const collision::World& movementWorld() const;
    const collision::World& staticWorld() const;
    const collision::World& projectileLeaves() const;
private:
    const collision::World& staticWorld_;
    const std::vector<doors::Door>& doors_;
    game_lab::Level& level_;
    collision::World movementWorld_;
    collision::World projectileLeaves_;
    doors::dos::Counter counter_;
    bool active_ = false;
};
