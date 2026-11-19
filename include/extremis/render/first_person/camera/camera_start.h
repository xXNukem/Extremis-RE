#pragma once

#include "extremis/gameplay/player/camera.h"

namespace game_lab { struct Level; }

// Selects an interior sample with the greatest clearance, then faces the longest
// finite sightline. Throws if no polygon interior clear of walls can be found.
Camera initialCamera(const game_lab::Level& level);
