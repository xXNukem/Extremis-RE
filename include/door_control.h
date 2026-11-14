#pragma once

#include "camera.h"
#include "door.h"
#include "door_clock.h"
#include "player_collision.h"

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
    void rebuildWorld();
    enum class CardResult { Unlocked, AlreadyDisabled, Rejected, NoCard, InvalidTarget };
    CardResult useCardReader(std::uint16_t parameter, std::optional<std::uint16_t> card);
    const collision::World& movementWorld() const { return movementWorld_; }
    const collision::World& staticWorld() const { return staticWorld_; }
private:
    const collision::World& staticWorld_;
    const std::vector<doors::Door>& doors_;
    game_lab::Level& level_;
    collision::World movementWorld_;
    doors::dos::Counter counter_;
    bool active_ = false;
};
