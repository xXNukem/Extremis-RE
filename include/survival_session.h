#pragma once

#include "player_vitals.h"
#include "player_inventory.h"
#include "original_hud.h"
#include "game_messages.h"

#include <cstdint>
#include <string>

union SDL_Event;
class GameAudio;

// Runtime adapter: gameplay state stays in Vitals; HUD and audio only observe it.
class SurvivalSession {
public:
    explicit SurvivalSession(bool diagnostics = false);
    void handleEvent(const SDL_Event& event, std::uint32_t windowId);
    bool update(double seconds, bool simulationActive, GameAudio& audio, bool running = false);
    bool alive() const;
    hud::State hudState() const;
    std::string windowTitle(const std::string& base) const;
    player::Vitals& vitals();
    player::Inventory& inventory();
    const player::Inventory& inventory() const;
    void postMessage(messages::Id id);
    void clearMessages();
private:
    player::Vitals vitals_;
    player::Inventory inventory_;
    messages::Queue messages_;
    player::DeathCause announcedDeath_ = player::DeathCause::None;
    bool diagnostics_ = false;
    bool wasActive_ = false;
};
