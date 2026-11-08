#pragma once

#include "game_lab.h"

#include <array>
#include <optional>
#include <vector>

namespace doors {
struct Point { double x, y; };
struct Face {
    Point a, b;
    std::uint16_t material;
    double textureSpan = 1.0; // Fraction of original horizontal texture still visible.
};
enum class State { Closed, Opening, Open, Closing };
struct Door {
    std::uint16_t stateIndex;
    std::uint16_t originalState;
    std::array<Face, 2> faces;
    State state = State::Closed;
    double progress = 0.0; // 0 = closed, 1 = completely retracted.
};

// DOS 0x1184f: dynamic doors override the original connection word.
bool connectionEnabled(const game_lab::Level& level, std::uint16_t stateIndex,
                       const std::vector<Door>& dynamicDoors);
std::vector<bool> connectedSectors(const game_lab::Level& level, std::size_t start,
                                   const std::vector<Door>& dynamicDoors);

// Provisional: one second for a full stroke, independent of frame rate.
void update(Door& door, double elapsedSeconds);
void updateAll(std::vector<Door>& doors, double elapsedSeconds);
void toggle(Door& door);
void toggleAll(std::vector<Door>& doors);

// Original per-face retraction: A approaches B; columns [0, 60 * (1-progress)).
// The stored closed endpoints and material remain unchanged.
std::optional<Face> visibleFace(const Door& door, const Face& closedFace);

// Only paired, reversed connection faces with a demonstrated closed state.
// Missing states, open/partial states and unmatched connections remain passages.
std::vector<Door> closedDoors(const game_lab::Level& level);
} // namespace doors
