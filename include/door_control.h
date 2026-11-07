#pragma once

#include "camera.h"
#include "door.h"
#include "player_collision.h"

// Coordinates the existing doors with one player's collision world and use action.
// Borrows both collections; neither rendering nor SDL is involved.
class DoorControl {
public:
    enum class UseResult { None, Activated, Locked };
    enum class UseReason { NoCandidate, OutOfRange, Occluded, AccessCondition, Accepted };
    struct UseTrace {
        UseReason reason = UseReason::NoCandidate;
        std::optional<std::size_t> doorIndex;
        std::optional<std::uint16_t> doorId;
        double distance = 0;
        double range = 0;
        std::optional<double> obstacleDistance;
        doors::State before = doors::State::Closed;
        doors::State after = doors::State::Closed;
        double progress = 0;
    };
    DoorControl(const collision::World& staticWorld, std::vector<doors::Door>& doors);
    void resetLevel();
    void prepareFrame(const Camera* player, double seconds, const collision::Settings& body);
    UseResult interact(const Camera& player, double range = 200.0, UseTrace* trace = nullptr);
    enum class CardResult { Unlocked, AlreadyDisabled, Rejected, NoCard, InvalidTarget };
    CardResult useCardReader(std::uint16_t parameter, std::optional<std::uint16_t> card);
    const collision::World& movementWorld() const;
private:
    const collision::World& staticWorld_;
    std::vector<doors::Door>& doors_;
    std::vector<std::optional<std::size_t>> approachFaces_;
    collision::World movementWorld_;
    struct Target { std::size_t index; double distance; };
    std::optional<Target> findTarget(const Camera& player) const;
    UseResult activate(const Target& target, UseTrace& trace);
    void chooseApproach(std::size_t index, const Camera& player, double clearance);
    void advanceSafely(std::size_t index, const Camera* player, double seconds, double clearance);
    void rebuildWorld();
};
