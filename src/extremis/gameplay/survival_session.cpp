#include <cstdint>
#include <string>
#include "extremis/gameplay/player/survival_session.h"
#include "extremis/audio/game_audio.h"

#include <SDL3/SDL_log.h>
#include <algorithm>

SurvivalSession::SurvivalSession(bool diagnostics) : diagnostics_(diagnostics)
{
#ifdef EXTREMIS_DEBUG_MENU
    if (diagnostics_)
        SDL_Log("VITALS DEBUG enabled: F5 damage 250; F6 restore health; F7 oxygen +60 s; "
                "F8 exhaust oxygen; F9 full diagnostic bottle; F10 reset vitals only; F11 diagnostic battery + radar.");
#else
    diagnostics_=false;
#endif
}

void SurvivalSession::handleInput(SurvivalInput input)
{
    if (input.suspendClock) wasActive_ = false;
#ifdef EXTREMIS_DEBUG_MENU
    if (!diagnostics_) return;
    using Diagnostic = SurvivalInput::Diagnostic;
    switch (input.diagnostic) {
    case Diagnostic::Damage: vitals_.applyDamage(250); break; // Diagnostic amount, not an attack.
    case Diagnostic::RestoreHealth: vitals_.restoreHealth(); break;
    case Diagnostic::AddOxygen: vitals_.update(60,true); break;
    case Diagnostic::ExhaustOxygen: vitals_.update(1800,true); break;
    case Diagnostic::FillOxygenReserve: {
        int diagnosticBottle = player::Vitals::maximumOxygen;
        vitals_.exchangeOxygen(diagnosticBottle);
        break;
    }
    case Diagnostic::EnableDetector: {
        std::uint16_t battery=900;
        inventory_.takeSupply(battery);
        if (!inventory_.detectorEnabled()) inventory_.toggleDetector();
        break;
    }
    case Diagnostic::ResetVitals: vitals_.reset(); wasActive_ = false; break;
    case Diagnostic::None: return;
    }
    SDL_Log("VITALS DIAGNOSTIC: health=%d oxygen=%d alive=%s; F10 resets vitals only, not the world",
            vitals_.health(),vitals_.oxygen(),alive() ? "yes" : "no");
#endif
}

bool SurvivalSession::update(double seconds, bool simulationActive, GameAudio& audio, bool running, bool deferDeathSound,
                             std::uint16_t gameplayTicks)
{
    messages_.update(seconds,simulationActive && wasActive_ && alive());
    if (simulationActive && alive() && gameplayTicks && inventory_.batteryMode()!=player::BatteryMode::Off) {
        const auto previousMode=inventory_.batteryMode();
        inventory_.updateBattery(gameplayTicks);
        if (inventory_.batteryMode()==player::BatteryMode::Off)
            messages_.post(previousMode==player::BatteryMode::Radar ?
                           messages::Id::RadarExpired : messages::Id::OpticsExpired);
        if (inventory_.batteryCharge()<4800) messages_.post(messages::Id::BatteryLow);
    }
    // The vitals keep their existing elapsed-time path.
    vitals_.update(seconds,simulationActive && wasActive_,running);
    wasActive_ = simulationActive && alive();
    const auto death = vitals_.deathCause();
    if (death == announcedDeath_) return false;
    announcedDeath_ = death;
    if (death == player::DeathCause::None) audio.endDeath();
    else {
        if (!deferDeathSound) audio.beginDeath(death == player::DeathCause::Oxygen
                             ? GameAudio::DeathSound::Oxygen : GameAudio::DeathSound::Health);
        SDL_Log("Player death: %s; movement/use disabled.",
                death == player::DeathCause::Oxygen ? "oxygen" : "health");
    }
    return true;
}

bool SurvivalSession::alive() const { return vitals_.alive(); }

hud::State SurvivalSession::hudState() const
{
    hud::State result;
    result.health = vitals_.health();
    result.oxygen = vitals_.oxygen();
    result.messages = messages_.visible();
    result.batteryCharge=inventory_.batteryCharge(); result.batteryInUse=inventory_.batteryMode()!=player::BatteryMode::Off;
    result.weapon = inventory_.weapon();
    result.ammunition = inventory_.ammunition();
    result.oxygenBottle = inventory_.oxygenReserve() >= 0;
    result.cards = unsigned(std::count_if(inventory_.cards().begin(),inventory_.cards().end(),
                                         [](int slot){ return slot >= 0; }));
    result.injectors = unsigned(std::count_if(inventory_.injectors().begin(),inventory_.injectors().end(),
                                             [](int slot){ return slot >= 0; }));
    return result;
}

player::Vitals& SurvivalSession::vitals() { return vitals_; }
player::Inventory& SurvivalSession::inventory() { return inventory_; }
const player::Inventory& SurvivalSession::inventory() const { return inventory_; }
void SurvivalSession::postMessage(messages::Id id) { messages_.post(id); }
void SurvivalSession::postDestruction(std::int32_t remaining) { messages_.postDestruction(remaining); }
void SurvivalSession::clearMessages() { messages_.clear(); }

std::string SurvivalSession::windowTitle(const std::string& base) const
{
    std::string result = base;
#ifdef EXTREMIS_DEBUG_MENU
    if (diagnostics_) result += " | VITALS DEBUG";
#endif
    if (!alive()) result += " | DEAD";
    return result;
}
