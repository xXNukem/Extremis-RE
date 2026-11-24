#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include "extremis/ui/original_map.h"
#include <SDL3/SDL_events.h>

class SurvivalSession;
namespace original_map {
class Session {
public:
    void beginFrame(unsigned zone);
    bool handleEvent(const SDL_Event& event, std::uint32_t windowId, const Camera* camera,
                     bool gameplay, bool transferred, bool blocked, bool resourcesAvailable,
                     SurvivalSession& player);
    void update(double seconds, bool focused);
    void clearInput();
    bool modal() const;
    bool blocksSimulation() const;
    const State* state() const;
private:
    std::optional<State> state_;
    unsigned zone_=0;
    double ticks_=0;
    bool transitioned_=false;
    std::array<bool,SDL_SCANCODE_COUNT> keys_{};
};
}
