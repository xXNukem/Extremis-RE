#include "door.h"

#include <algorithm>
#include <cmath>

namespace doors {
void dos::updateTable(std::vector<std::uint16_t>& states, std::uint16_t counter)
{
    const auto delta = increment(counter);
    for (auto& state : states) state = update(state, delta);
}

std::uint16_t dos::update(std::uint16_t state, std::uint8_t delta)
{
    auto value = phase(state);
    if (state & opening) {
        value = static_cast<std::uint8_t>(value + delta); // ADD AL,BL wraps first.
        // CMP AL,90 / JL: signed byte comparison, without signed narrowing.
        if (value >= 90 && value < 128) {
            value = 90;
            state ^= opening; // Discard excess; no descending step this call.
        }
    } else {
        value = static_cast<std::uint8_t>(value - delta);
        if (value & 0x80) value = 0; // SUB AL,BL / JNS tests SF, not borrow.
    }
    return static_cast<std::uint16_t>((state & 0xff00) | value);
}

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

bool connectionEnabled(const game_lab::Level& level, std::uint16_t stateIndex)
{
    return stateIndex < level.connectionStates.size() &&
           dos::connected(level.connectionStates[stateIndex]);
}

std::vector<bool> connectedSectors(const game_lab::Level& level, std::size_t start)
{
    std::vector<bool> visible(level.sectors.size(), false);
    if (start >= visible.size()) return visible;
    std::vector<std::size_t> pending{start};
    visible[start] = true;
    while (!pending.empty()) {
        const auto current = pending.back();
        pending.pop_back();
        const auto& sector = level.sectors[current];
        // DOS: connection wall i corresponds to neighbor i, before geometric clipping.
        for (std::size_t i = 0; i < sector.neighbors.size() && i < sector.walls.size(); ++i) {
            const auto next = sector.neighbors[i];
            if (next >= visible.size() || visible[next] ||
                !connectionEnabled(level, sector.walls[i].extra)) continue;
            visible[next] = true;
            pending.push_back(next);
        }
    }
    return visible;
}

std::optional<Face> visibleFace(const game_lab::Level& level, const Door& door, const Face& closedFace)
{
    if (door.stateIndex >= level.connectionStates.size()) return std::nullopt;
    const auto phase = dos::phase(level.connectionStates[door.stateIndex]);
    if (phase >= 60) return std::nullopt;
    const double span = double(60 - phase) / 60;
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
                {faces[i].face, faces[match].face}});
        }
    }
    return result;
}
} // namespace doors
