#pragma once

#include "extremis/gameplay/player/player_vitals.h"
#include "extremis/gameplay/player/player_inventory.h"
#include "extremis/ui/original_hud.h"
#include "extremis/ui/game_messages.h"
#include "extremis/gameplay/player/survival_actions.h"

#include <cstdint>
#include <string>

class GameAudio;

// Runtime adapter: gameplay state stays in Vitals; HUD and audio only observe it.
class SurvivalSession {
public:
    explicit SurvivalSession(bool diagnostics = false);
    void handleInput(SurvivalInput input);
    bool update(double seconds, bool simulationActive, GameAudio& audio, bool running = false,
                bool deferDeathSound = false, std::uint16_t gameplayTicks = 0);
    bool alive() const;
    hud::State hudState() const;
    std::string windowTitle(const std::string& base) const;
    player::Vitals& vitals();
    player::Inventory& inventory();
    const player::Inventory& inventory() const;
    void postMessage(messages::Id id);
    void postDestruction(std::int32_t remaining);
    void clearMessages();
private:
    player::Vitals vitals_;
    player::Inventory inventory_;
    messages::Queue messages_;
    player::DeathCause announcedDeath_ = player::DeathCause::None;
    bool diagnostics_ = false;
    bool wasActive_ = false;
};
