#include "survival_session.h"
#include "game_audio.h"

#include <SDL3/SDL.h>
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

void SurvivalSession::handleEvent(const SDL_Event& event, std::uint32_t windowId)
{
    if ((event.type == SDL_EVENT_WINDOW_FOCUS_LOST || event.type == SDL_EVENT_WINDOW_FOCUS_GAINED) &&
        event.window.windowID == windowId) wasActive_ = false;
#ifdef EXTREMIS_DEBUG_MENU
    if (event.type == SDL_EVENT_KEY_DOWN && event.key.windowID == windowId &&
        event.key.key == SDLK_TAB && !event.key.repeat) wasActive_ = false;
    if (!diagnostics_ || event.type != SDL_EVENT_KEY_DOWN || event.key.repeat ||
        event.key.windowID != windowId) return;
    switch (event.key.scancode) {
    case SDL_SCANCODE_F5: vitals_.applyDamage(250); break; // Diagnostic amount, not an attack.
    case SDL_SCANCODE_F6: vitals_.restoreHealth(); break;
    case SDL_SCANCODE_F7: vitals_.update(60,true); break;
    case SDL_SCANCODE_F8: vitals_.update(1800,true); break;
    case SDL_SCANCODE_F9: {
        int diagnosticBottle = player::Vitals::maximumOxygen;
        vitals_.exchangeOxygen(diagnosticBottle);
        break;
    }
    case SDL_SCANCODE_F11: {
        std::uint16_t battery=900;
        inventory_.takeSupply(battery);
        if (!inventory_.detectorEnabled()) inventory_.toggleDetector();
        break;
    }
    case SDL_SCANCODE_F10: vitals_.reset(); wasActive_ = false; break;
    default: return;
    }
    SDL_Log("VITALS DIAGNOSTIC: health=%d oxygen=%d alive=%s; F10 resets vitals only, not the world",
            vitals_.health(),vitals_.oxygen(),alive() ? "yes" : "no");
#endif
}

bool SurvivalSession::update(double seconds, bool simulationActive, GameAudio& audio, bool running)
{
    messages_.update(seconds,simulationActive && wasActive_ && alive());
    // Discard entry/resume frames, whose elapsed interval may include inactive time.
    inventory_.updateBattery(seconds,simulationActive && wasActive_ && alive());
    vitals_.update(seconds,simulationActive && wasActive_,running);
    wasActive_ = simulationActive && alive();
    const auto death = vitals_.deathCause();
    if (death == announcedDeath_) return false;
    announcedDeath_ = death;
    if (death == player::DeathCause::None) audio.endDeath();
    else {
        audio.beginDeath(death == player::DeathCause::Oxygen
                             ? GameAudio::DeathSound::Oxygen : GameAudio::DeathSound::Health);
        SDL_Log("Player death: %s. Original visual transition pending; movement/use disabled.",
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
    result.batteryCharge=inventory_.batteryCharge(); result.detectorEnabled=inventory_.detectorEnabled();
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
void SurvivalSession::clearMessages() { messages_.clear(); }

std::string SurvivalSession::windowTitle(const std::string& base) const
{
    std::string result = base;
#ifdef EXTREMIS_DEBUG_MENU
    if (diagnostics_) result += " | VITALS DEBUG";
#endif
    if (!alive()) result += " | DEAD - original visual transition pending";
    return result;
}
