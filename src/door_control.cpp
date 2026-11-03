#include "door_control.h"

#include <cmath>
#include <limits>

namespace {
collision::Segment segment(const doors::Face& face)
{
    return {{face.a.x, face.a.y}, {face.b.x, face.b.y}};
}

} // namespace

DoorControl::DoorControl(const collision::World& staticWorld, std::vector<doors::Door>& doors)
    : staticWorld_(staticWorld), doors_(doors), approachFaces_(doors.size()),
      movementWorld_(staticWorld) {}

const collision::World& DoorControl::movementWorld() const
{
    return movementWorld_;
}

DoorControl::CardResult DoorControl::useCardReader(std::uint16_t parameter,
                                                 std::optional<std::uint16_t> card)
{
    for (auto& door : doors_) {
        if (door.stateIndex != (parameter & 0xff)) continue;
        if (!(door.originalState & 0x0400)) return CardResult::AlreadyDisabled;
        if (!card) return CardResult::NoCard;
        // 0x1afc8..0x1afec filters C cards; 0x9488 / 0x95a1 compares low byte.
        if ((*card & 0xf000) != 0x1000 || (*card & 0xff) != (parameter >> 8))
            return CardResult::Rejected;
        door.originalState ^= 0x0400; // 0x95c2..0x95d7. Unlock, do not open or consume card.
        return CardResult::Unlocked;
    }
    return CardResult::InvalidTarget;
}

void DoorControl::chooseApproach(std::size_t index, const Camera& player, double clearance)
{
    const double distance = collision::signedDistanceToLine(segment(doors_[index].faces[0]),
                                                            {player.x, player.y});
    auto& approach = approachFaces_[index];
    // Keep the entering face until the whole circular body clears the door plane.
    if (!approach || std::abs(distance) > clearance) approach = distance <= 0 ? 0 : 1;
}

void DoorControl::advanceSafely(std::size_t index, const Camera* player,
                               double seconds, double clearance)
{
    auto& door = doors_[index];
    auto next = door;
    doors::update(next, seconds);
    if (player && door.state == doors::State::Closing) {
        const auto face = doors::visibleFace(next, next.faces[approachFaces_[index].value_or(0)]);
        // A closing face grows monotonically: its prospective segment contains its swept area.
        if (face && collision::overlaps(segment(*face), {player->x, player->y}, clearance)) {
            door.state = doors::State::Opening; // Preserve progress; never push or crush the player.
            return;
        }
    }
    door = next;
}

void DoorControl::rebuildWorld()
{
    movementWorld_ = staticWorld_;
    for (std::size_t i = 0; i < doors_.size(); ++i) {
        const auto& door = doors_[i];
        const auto face = doors::visibleFace(door, door.faces[approachFaces_[i].value_or(0)]);
        if (face) movementWorld_.push_back(segment(*face));
    }
}

void DoorControl::prepareFrame(const Camera* player, double seconds, const collision::Settings& body)
{
    approachFaces_.resize(doors_.size());
    const double clearance = body.radius + body.skin;
    for (std::size_t i = 0; i < doors_.size(); ++i) {
        if (player) chooseApproach(i, *player, clearance);
        advanceSafely(i, player, seconds, clearance);
    }
    rebuildWorld();
}

std::optional<DoorControl::Target> DoorControl::findTarget(const Camera& player) const
{
    const collision::Point start{player.x, player.y};
    const collision::Point direction{std::cos(player.angle), std::sin(player.angle)};
    std::optional<Target> nearest;
    for (std::size_t i = 0; i < doors_.size(); ++i) {
        // Target the original aperture even when the leaf is fully retracted.
        const auto hit = collision::rayDistance(segment(doors_[i].faces[0]), start, direction,
            nearest ? nearest->distance : std::numeric_limits<double>::infinity());
        if (hit && (!nearest || *hit < nearest->distance)) {
            nearest = Target{i, *hit};
        }
    }
    return nearest;
}

DoorControl::UseResult DoorControl::activate(const Target& target, UseTrace& trace)
{
    auto& door = doors_[target.index];
    // Original 0x0400 gate: no guessed inventory or automatic access bypass for normal use.
    if (door.originalState & 0x0400) {
        trace.reason = UseReason::AccessCondition;
        return UseResult::Locked;
    }
    doors::toggle(door);
    trace.after = door.state;
    trace.reason = UseReason::Accepted;
    return UseResult::Activated;
}

DoorControl::UseResult DoorControl::interact(const Camera& player, double range, UseTrace* output)
{
    UseTrace local;
    auto& trace = output ? *output : local;
    trace = UseTrace{};
    trace.range = range;
    const auto target = findTarget(player);
    if (!target) return UseResult::None;
    const auto& door = doors_[target->index];
    trace.doorIndex = target->index;
    trace.doorId = door.stateIndex;
    trace.distance = target->distance;
    trace.before = trace.after = door.state;
    trace.progress = door.progress;
    if (target->distance > range) {
        trace.reason = UseReason::OutOfRange;
        return UseResult::None;
    }
    trace.obstacleDistance = collision::rayDistance(movementWorld_, {player.x, player.y},
        {std::cos(player.angle), std::sin(player.angle)}, target->distance);
    if (trace.obstacleDistance && *trace.obstacleDistance < target->distance - 1e-6) {
        trace.reason = UseReason::Occluded;
        return UseResult::None;
    }
    return activate(*target, trace);
}
