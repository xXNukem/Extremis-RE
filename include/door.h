#pragma once

#include "game_lab.h"

#include <array>
#include <optional>
#include <vector>

namespace doors {
// Original DOS connection-word logic. Level::connectionStates owns every word.
namespace dos {
constexpr std::uint16_t opening = 0x0100;
constexpr std::uint16_t locked = 0x0400;
constexpr std::uint8_t phase(std::uint16_t state) { return state & 0xff; }
// DOS 0x11484–0x1148b: BL after shifting the original counter word.
constexpr std::uint8_t increment(std::uint16_t counter) { return (counter >> 1) & 0xff; }
std::uint16_t update(std::uint16_t state, std::uint8_t delta);
// One snapshot, one update per entry; neither reads a clock nor resets it.
void updateTable(std::vector<std::uint16_t>& states, std::uint16_t counter);
constexpr bool leafVisible(std::uint16_t state) { return phase(state) < 60; }
constexpr bool connected(std::uint16_t state) { return (state & 0x01ff) != 0; }
// Entry condition only: movement geometry still decides whether passage occurs.
constexpr bool passageBranchEnabled(std::uint16_t state) { return (state & 0x01ff) > 0x013c; }
} // namespace dos

struct Point { double x, y; };
struct Face {
    Point a, b;
    std::uint16_t material;
    double textureSpan = 1.0; // Fraction of original horizontal texture still visible.
};
struct Door {
    std::uint16_t stateIndex;
    std::array<Face, 2> faces;
};

bool connectionEnabled(const game_lab::Level& level, std::uint16_t stateIndex);
std::vector<bool> connectedSectors(const game_lab::Level& level, std::size_t start);
// Both faces obtain their phase from the same table entry.
std::optional<Face> visibleFace(const game_lab::Level& level, const Door& door,
                                const Face& closedFace);

// Only paired, reversed connection faces with a demonstrated closed state.
// Missing states, open/partial states and unmatched connections remain passages.
std::vector<Door> closedDoors(const game_lab::Level& level);
} // namespace doors
