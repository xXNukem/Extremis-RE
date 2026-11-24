#include <cstdint>
#include "platform/sdl/input/survival_input.h"

#include <SDL3/SDL_events.h>

namespace platform::sdl {
SurvivalInput survivalInput(const SDL_Event& event, std::uint32_t windowId)
{
    if ((event.type == SDL_EVENT_WINDOW_FOCUS_LOST || event.type == SDL_EVENT_WINDOW_FOCUS_GAINED) &&
        event.window.windowID == windowId) return {true};

#ifdef EXTREMIS_DEBUG_MENU
    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat && event.key.windowID == windowId) {
        SurvivalInput input;
        input.suspendClock = event.key.key == SDLK_TAB;
        using Diagnostic = SurvivalInput::Diagnostic;
        switch (event.key.scancode) {
        case SDL_SCANCODE_F5: input.diagnostic = Diagnostic::Damage; break;
        case SDL_SCANCODE_F6: input.diagnostic = Diagnostic::RestoreHealth; break;
        case SDL_SCANCODE_F7: input.diagnostic = Diagnostic::AddOxygen; break;
        case SDL_SCANCODE_F8: input.diagnostic = Diagnostic::ExhaustOxygen; break;
        case SDL_SCANCODE_F9: input.diagnostic = Diagnostic::FillOxygenReserve; break;
        case SDL_SCANCODE_F10: input.diagnostic = Diagnostic::ResetVitals; break;
        case SDL_SCANCODE_F11: input.diagnostic = Diagnostic::EnableDetector; break;
        default: break;
        }
        return input;
    }
#endif
    return {};
}
}
