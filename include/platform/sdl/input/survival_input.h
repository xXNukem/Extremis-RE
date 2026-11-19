#pragma once

#include "extremis/gameplay/player/survival_actions.h"

#include <cstdint>

union SDL_Event;

namespace platform::sdl {
SurvivalInput survivalInput(const SDL_Event& event, std::uint32_t windowId);
}
