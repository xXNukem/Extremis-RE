#include "door.h"

#include <algorithm>
#include <cmath>

namespace doors {
namespace {
Point vertex(const game_lab::Sector& sector, std::uint16_t index)
{
    const auto& v = sector.vertices[index];
    return {double(sector.gx) + v.x, double(sector.gy) + v.y};
}

bool same(Point a, Point b) { return a.x == b.x && a.y == b.y; }

struct Candidate {
    std::size_t sector;
    std::uint16_t stateIndex;
    Face face;
};

std::vector<Candidate> candidates(const game_lab::Level& level)
{
    std::vector<Candidate> result;
    for (std::size_t i = 0; i < level.sectors.size(); ++i) {
        const auto& sector = level.sectors[i];
        for (const auto& wall : sector.walls) {
            if ((wall.type & 0xf000) != 0x1000 ||
                wall.extra >= level.connectionStates.size()) continue;
            const auto state = level.connectionStates[wall.extra];
            // Only the two closed words observed in level 1 are supported.
            if (state != 0 && state != 0x0400) continue;
            const auto a = vertex(sector, wall.vertexA), b = vertex(sector, wall.vertexB);
            if (!same(a, b)) result.push_back({i, wall.extra, {a, b,
                static_cast<std::uint16_t>(wall.type & 0xff)}});
        }
    }
    return result;
}

bool reversedPair(const Candidate& a, const Candidate& b)
{
    return a.sector != b.sector && a.stateIndex == b.stateIndex &&
           same(a.face.a, b.face.b) && same(a.face.b, b.face.a);
}
} // namespace

void toggle(Door& door)
{
    door.state = door.state == State::Closed || door.state == State::Closing
        ? State::Opening : State::Closing;
}

void toggleAll(std::vector<Door>& doors)
{
    for (auto& door : doors) toggle(door);
}

void update(Door& door, double elapsedSeconds)
{
    if (!std::isfinite(elapsedSeconds) || elapsedSeconds <= 0) return;
    constexpr double strokeSeconds = 1.0; // Original speed has not been established.
    if (door.state != State::Opening && door.state != State::Closing) return;
    const double direction = door.state == State::Opening ? 1.0 : -1.0;
    door.progress = std::clamp(door.progress + direction * elapsedSeconds / strokeSeconds,
                               0.0, 1.0);
    // Snap only round-off at the endpoints (e.g. sixty updates of 1/60 second).
    if (door.progress <= 1e-12) {
        door.progress = 0.0;
        door.state = State::Closed;
    }
    if (door.progress >= 1.0 - 1e-12) {
        door.progress = 1.0;
        door.state = State::Open;
    }
}

void updateAll(std::vector<Door>& doors, double elapsedSeconds)
{
    for (auto& door : doors) update(door, elapsedSeconds);
}

std::optional<Face> visibleFace(const Door& door, const Face& closedFace)
{
    if (door.state == State::Open || door.progress >= 1.0) return std::nullopt;
    const double span = 1.0 - std::clamp(door.progress, 0.0, 1.0);
    return Face{{closedFace.b.x + (closedFace.a.x - closedFace.b.x) * span,
                 closedFace.b.y + (closedFace.a.y - closedFace.b.y) * span},
                closedFace.b, closedFace.material, span};
}

std::vector<Door> closedDoors(const game_lab::Level& level)
{
    const auto faces = candidates(level);
    std::vector<Door> result;
    for (std::size_t i = 0; i < faces.size(); ++i) {
        std::size_t match = faces.size(), count = 0;
        for (std::size_t j = 0; j < faces.size(); ++j) {
            if (faces[j].stateIndex == faces[i].stateIndex) ++count;
            if (reversedPair(faces[i], faces[j])) match = j;
        }
        // Ambiguous IDs are deliberately not rendered as guessed doors.
        if (count == 2 && match > i && match < faces.size()) {
            result.push_back({faces[i].stateIndex,
                level.connectionStates[faces[i].stateIndex],
                {faces[i].face, faces[match].face}});
        }
    }
    return result;
}
} // namespace doors
